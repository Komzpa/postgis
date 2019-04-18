/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * PostGIS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * PostGIS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PostGIS.  If not, see <http://www.gnu.org/licenses/>.
 *
 **********************************************************************
 *
 * Copyright 2009 Paul Ramsey <pramsey@opengeo.org>
 *
 **********************************************************************/


#include "postgres.h"
#include "fmgr.h"
#include "funcapi.h"
#include "access/tupmacs.h"
#include "utils/datum.h"
#include "utils/array.h"
#include "utils/lsyscache.h"

#include "../postgis_config.h"

#include "liblwgeom.h"
#include "lwgeom_geos.h"
#include "lwgeom_pg.h"
#include "lwgeom_transform.h"

/* Local prototypes */
Datum PGISDirectFunctionCall1(PGFunction func, Datum arg1);
Datum PGISDirectFunctionCall2(PGFunction func, Datum arg1, Datum arg2);
Datum pgis_geometry_accum_transfn(PG_FUNCTION_ARGS);
Datum pgis_geometry_union_finalfn(PG_FUNCTION_ARGS);
Datum pgis_geometry_collect_finalfn(PG_FUNCTION_ARGS);
Datum pgis_geometry_polygonize_finalfn(PG_FUNCTION_ARGS);
Datum pgis_geometry_makeline_finalfn(PG_FUNCTION_ARGS);
Datum pgis_geometry_clusterintersecting_finalfn(PG_FUNCTION_ARGS);
Datum pgis_geometry_clusterwithin_finalfn(PG_FUNCTION_ARGS);

/* External prototypes */
Datum pgis_union_geometry_array(PG_FUNCTION_ARGS);
Datum LWGEOM_collect_garray(PG_FUNCTION_ARGS);
Datum polygonize_garray(PG_FUNCTION_ARGS);
Datum clusterintersecting_garray(PG_FUNCTION_ARGS);
Datum cluster_within_distance_garray(PG_FUNCTION_ARGS);
Datum LWGEOM_makeline_garray(PG_FUNCTION_ARGS);


/** @file
** Versions of PostgreSQL < 8.4 perform array accumulation internally using
** pass by value, which is very slow working with large/many geometries.
** Hence PostGIS currently implements its own aggregate for building
** geometry arrays using pass by reference, which is significantly faster and
** similar to the method used in PostgreSQL 8.4.
**
** Hence we can revert this to the original aggregate functions from 1.3 at
** whatever point PostgreSQL 8.4 becomes the minimum version we support :)
*/


/**
** To pass the internal ArrayBuildState pointer between the
** transfn and finalfn we need to wrap it into a custom type first,
** the pgis_abs type in our case.  The extra "data" member can optionally
** be used to pass an additional constant argument to a finalizer function.
*/

typedef struct
{
	ArrayBuildState *a;
	Datum data;
}
pgis_abs;


/**
** The transfer function hooks into the PostgreSQL accumArrayResult()
** function (present since 8.0) to build an array in a side memory
** context.
*/
PG_FUNCTION_INFO_V1(pgis_geometry_accum_transfn);
Datum
pgis_geometry_accum_transfn(PG_FUNCTION_ARGS)
{
	Oid arg1_typeid = get_fn_expr_argtype(fcinfo->flinfo, 1);
	MemoryContext aggcontext;
	ArrayBuildState *state;
	pgis_abs *p;
	Datum elem;

	if (arg1_typeid == InvalidOid)
		ereport(ERROR,
		        (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
		         errmsg("could not determine input data type")));

	if ( ! AggCheckCallContext(fcinfo, &aggcontext) )
	{
		/* cannot be called directly because of dummy-type argument */
		elog(ERROR, "%s called in non-aggregate context", __func__);
		aggcontext = NULL;  /* keep compiler quiet */
	}

	if ( PG_ARGISNULL(0) )
	{
		MemoryContext old = MemoryContextSwitchTo(aggcontext);
		p = (pgis_abs*) palloc(sizeof(pgis_abs));
		p->a = NULL;
		p->data = (Datum) NULL;

		if (PG_NARGS() == 3)
		{
			Datum argument = PG_GETARG_DATUM(2);
			Oid dataOid = get_fn_expr_argtype(fcinfo->flinfo, 2);

			p->data = datumCopy(argument, get_typbyval(dataOid), get_typlen(dataOid));

		}
		MemoryContextSwitchTo(old);
	}
	else
	{
		p = (pgis_abs*) PG_GETARG_POINTER(0);
	}
	state = p->a;
	elem = PG_ARGISNULL(1) ? (Datum) 0 : PG_GETARG_DATUM(1);
	state = accumArrayResult(state,
	                         elem,
	                         PG_ARGISNULL(1),
	                         arg1_typeid,
	                         aggcontext);
	p->a = state;

	PG_RETURN_POINTER(p);
}



Datum pgis_accum_finalfn(pgis_abs *p, MemoryContext mctx, FunctionCallInfo fcinfo);

/**
** The final function rescues the built array from the side memory context
** using the PostgreSQL built-in function makeMdArrayResult
*/
Datum
pgis_accum_finalfn(pgis_abs *p, MemoryContext mctx, __attribute__((__unused__)) FunctionCallInfo fcinfo)
{
	int dims[1];
	int lbs[1];
	ArrayBuildState *state;
	Datum result;

	/* cannot be called directly because of internal-type argument */
	Assert(fcinfo->context &&
	       (IsA(fcinfo->context, AggState) ||
	        IsA(fcinfo->context, WindowAggState))
	       );

	state = p->a;
	dims[0] = state->nelems;
	lbs[0] = 1;
	result = makeMdArrayResult(state, 1, dims, lbs, mctx, false);
	return result;
}

/**
* The "collect" final function passes the geometry[] to a geometrycollection
* conversion before returning the result.
*/
PG_FUNCTION_INFO_V1(pgis_geometry_collect_finalfn);
Datum
pgis_geometry_collect_finalfn(PG_FUNCTION_ARGS)
{
	pgis_abs *p;
	Datum result = 0;
	Datum geometry_array = 0;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();   /* returns null iff no input values */

	p = (pgis_abs*) PG_GETARG_POINTER(0);

	geometry_array = pgis_accum_finalfn(p, CurrentMemoryContext, fcinfo);
	result = PGISDirectFunctionCall1( LWGEOM_collect_garray, geometry_array );
	if (!result)
		PG_RETURN_NULL();

	PG_RETURN_DATUM(result);
}


/**
* The "polygonize" final function passes the geometry[] to a polygonization
* before returning the result.
*/
PG_FUNCTION_INFO_V1(pgis_geometry_polygonize_finalfn);
Datum
pgis_geometry_polygonize_finalfn(PG_FUNCTION_ARGS)
{
	pgis_abs *p;
	Datum result = 0;
	Datum geometry_array = 0;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();   /* returns null iff no input values */

	p = (pgis_abs*) PG_GETARG_POINTER(0);

	geometry_array = pgis_accum_finalfn(p, CurrentMemoryContext, fcinfo);
	result = PGISDirectFunctionCall1( polygonize_garray, geometry_array );
	if (!result)
		PG_RETURN_NULL();

	PG_RETURN_DATUM(result);
}

/**
* The "makeline" final function passes the geometry[] to a line builder
* before returning the result.
*/
PG_FUNCTION_INFO_V1(pgis_geometry_makeline_finalfn);
Datum
pgis_geometry_makeline_finalfn(PG_FUNCTION_ARGS)
{
	pgis_abs *p;
	Datum result = 0;
	Datum geometry_array = 0;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();   /* returns null iff no input values */

	p = (pgis_abs*) PG_GETARG_POINTER(0);

	geometry_array = pgis_accum_finalfn(p, CurrentMemoryContext, fcinfo);
	result = PGISDirectFunctionCall1( LWGEOM_makeline_garray, geometry_array );
	if (!result)
		PG_RETURN_NULL();

	PG_RETURN_DATUM(result);
}

/**
 * The "clusterintersecting" final function passes the geometry[] to a
 * clustering function before returning the result.
*/
PG_FUNCTION_INFO_V1(pgis_geometry_clusterintersecting_finalfn);
Datum
pgis_geometry_clusterintersecting_finalfn(PG_FUNCTION_ARGS)
{
	pgis_abs *p;
	Datum result = 0;
	Datum geometry_array = 0;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	p = (pgis_abs*) PG_GETARG_POINTER(0);
	geometry_array = pgis_accum_finalfn(p, CurrentMemoryContext, fcinfo);
	result = PGISDirectFunctionCall1( clusterintersecting_garray, geometry_array );
	if (!result)
		PG_RETURN_NULL();

	PG_RETURN_DATUM(result);
}

/**
 * The "cluster_within_distance" final function passes the geometry[] to a
 * clustering function before returning the result.
*/
PG_FUNCTION_INFO_V1(pgis_geometry_clusterwithin_finalfn);
Datum
pgis_geometry_clusterwithin_finalfn(PG_FUNCTION_ARGS)
{
	pgis_abs *p;
	Datum result = 0;
	Datum geometry_array = 0;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	p = (pgis_abs*) PG_GETARG_POINTER(0);

	if (!p->data)
	{
		elog(ERROR, "Tolerance not defined");
		PG_RETURN_NULL();
	}

	geometry_array = pgis_accum_finalfn(p, CurrentMemoryContext, fcinfo);
	result = PGISDirectFunctionCall2( cluster_within_distance_garray, geometry_array, p->data);
	if (!result)
		PG_RETURN_NULL();

	PG_RETURN_DATUM(result);
}

/**
* A modified version of PostgreSQL's DirectFunctionCall1 which allows NULL results; this
* is required for aggregates that return NULL.
*/
Datum
PGISDirectFunctionCall1(PGFunction func, Datum arg1)
{
#if POSTGIS_PGSQL_VERSION < 120
	FunctionCallInfoData fcinfo;
	Datum           result;


	InitFunctionCallInfoData(fcinfo, NULL, 1, InvalidOid, NULL, NULL);


	fcinfo.arg[0] = arg1;
	fcinfo.argnull[0] = false;

	result = (*func) (&fcinfo);

	/* Check for null result, returning a "NULL" Datum if indicated */
	if (fcinfo.isnull)
		return (Datum) 0;

	return result;
#else
	LOCAL_FCINFO(fcinfo, 1);
	Datum result;

	InitFunctionCallInfoData(*fcinfo, NULL, 1, InvalidOid, NULL, NULL);

	fcinfo->args[0].value = arg1;
	fcinfo->args[0].isnull = false;

	result = (*func)(fcinfo);

	/* Check for null result, returning a "NULL" Datum if indicated */
	if (fcinfo->isnull)
		return (Datum)0;

	return result;
#endif /* POSTGIS_PGSQL_VERSION < 120 */
}

/**
* A modified version of PostgreSQL's DirectFunctionCall2 which allows NULL results; this
* is required for aggregates that return NULL.
*/
Datum
PGISDirectFunctionCall2(PGFunction func, Datum arg1, Datum arg2)
{
#if POSTGIS_PGSQL_VERSION < 120
	FunctionCallInfoData fcinfo;
	Datum           result;

	InitFunctionCallInfoData(fcinfo, NULL, 2, InvalidOid, NULL, NULL);

	fcinfo.arg[0] = arg1;
	fcinfo.arg[1] = arg2;
	fcinfo.argnull[0] = false;
	fcinfo.argnull[1] = false;

	result = (*func) (&fcinfo);

	/* Check for null result, returning a "NULL" Datum if indicated */
	if (fcinfo.isnull)
		return (Datum) 0;

	return result;
#else
	LOCAL_FCINFO(fcinfo, 2);
	Datum result;

	InitFunctionCallInfoData(*fcinfo, NULL, 2, InvalidOid, NULL, NULL);

	fcinfo->args[0].value = arg1;
	fcinfo->args[1].value = arg2;
	fcinfo->args[0].isnull = false;
	fcinfo->args[1].isnull = false;

	result = (*func)(fcinfo);

	/* Check for null result, returning a "NULL" Datum if indicated */
	if (fcinfo->isnull)
		return (Datum)0;

	return result;
#endif /* POSTGIS_PGSQL_VERSION < 120 */
}
