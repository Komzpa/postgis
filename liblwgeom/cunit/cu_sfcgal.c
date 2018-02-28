/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CUnit/Basic.h"

#include "cu_tester.h"
#include "liblwgeom.h"

extern LWGEOM * lwgeom_sfcgal_noop(const LWGEOM * geom_in);

static void
test_sfcgal_noop(void)
{
	int		i;

	char	       *ewkt[] =
	{
		"POINT(0 0.2)",
		"LINESTRING(-1 -1,-1 2.5,2 2,2 -1)",
		"TRIANGLE((0 0,-1 1,0 -1,0 0))",
		"MULTIPOINT(0.9 0.9,0.9 0.9,0.9 0.9,0.9 0.9,0.9 0.9,0.9 0.9)",
		"SRID=1;MULTILINESTRING((-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1))",
		"SRID=1;MULTILINESTRING((-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1))",
		"POLYGON((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0))",
		"SRID=4326;POLYGON((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0))",
		"SRID=4326;POLYGON((-1 -1 1,-1 2.5 1,2 2 2,2 -1 2,-1 -1 2),(0 0 1,0 1 1,1 1 1,1 0 2,0 0 2))",
		"SRID=4326;POLYGON((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5))",
		"SRID=100000;POLYGON((-1 -1 3,-1 2.5 3,2 2 3,2 -1 3,-1 -1 3),(0 0 3,0 1 3,1 1 3,1 0 3,0 0 3),(-0.5 -0.5 3,-0.5 -0.4 3,-0.4 -0.4 3,-0.4 -0.5 3,-0.5 -0.5 3))",
		"SRID=4326;MULTIPOLYGON(((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5)),((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5)))",
		"SRID=4326;GEOMETRYCOLLECTION(POINT(0 1),POLYGON((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0)),MULTIPOLYGON(((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5))))",
		"POLYHEDRALSURFACE(((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5)),((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5)))",
		"POLYHEDRALSURFACE(((-1 -1 1,-1 2.5 1,2 2 1,2 -1 1,-1 -1 1),(0 0 1,0 1 1,1 1 1,1 0 1,0 0 1),(-0.5 -0.5 1,-0.5 -0.4 1,-0.4 -0.4 1,-0.4 -0.5 1,-0.5 -0.5 1)),((-1 -1 1,-1 2.5 1,2 2 1,2 -1 1,-1 -1 1),(0 0 1,0 1 1,1 1 1,1 0 1,0 0 1),(-0.5 -0.5 1,-0.5 -0.4 1,-0.4 -0.4 1,-0.4 -0.5 1,-0.5 -0.5 1)))",
		"TIN(((0 0,0 -1,-1 1,0 0)),((0 0,1 0,0 -1,0 0)))",
	};

	char	       *expected_ewkt[] =
	{
		"POINT(0 0.2)",
		"LINESTRING(-1 -1,-1 2.5,2 2,2 -1)",
		"TRIANGLE((0 0,-1 1,0 -1,0 0))",
		"MULTIPOINT(0.9 0.9,0.9 0.9,0.9 0.9,0.9 0.9,0.9 0.9,0.9 0.9)",
		"SRID=1;MULTILINESTRING((-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1))",
		"SRID=1;MULTILINESTRING((-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1))",
		"POLYGON((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0))",
		"SRID=4326;POLYGON((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0))",
		"SRID=4326;POLYGON((-1 -1 1,-1 2.5 1,2 2 2,2 -1 2,-1 -1 2),(0 0 1,0 1 1,1 1 1,1 0 2,0 0 2))",
		"SRID=4326;POLYGON((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5))",
		"SRID=100000;POLYGON((-1 -1 3,-1 2.5 3,2 2 3,2 -1 3,-1 -1 3),(0 0 3,0 1 3,1 1 3,1 0 3,0 0 3),(-0.5 -0.5 3,-0.5 -0.4 3,-0.4 -0.4 3,-0.4 -0.5 3,-0.5 -0.5 3))",
		"SRID=4326;MULTIPOLYGON(((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5)),((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5)))",
		"SRID=4326;GEOMETRYCOLLECTION(POINT(0 1),POLYGON((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0)),MULTIPOLYGON(((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5))))",
		"POLYHEDRALSURFACE(((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5)),((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5)))",
		"POLYHEDRALSURFACE(((-1 -1 1,-1 2.5 1,2 2 1,2 -1 1,-1 -1 1),(0 0 1,0 1 1,1 1 1,1 0 1,0 0 1),(-0.5 -0.5 1,-0.5 -0.4 1,-0.4 -0.4 1,-0.4 -0.5 1,-0.5 -0.5 1)),((-1 -1 1,-1 2.5 1,2 2 1,2 -1 1,-1 -1 1),(0 0 1,0 1 1,1 1 1,1 0 1,0 0 1),(-0.5 -0.5 1,-0.5 -0.4 1,-0.4 -0.4 1,-0.4 -0.5 1,-0.5 -0.5 1)))",
		"TIN(((0 0,0 -1,-1 1,0 0)),((0 0,1 0,0 -1,0 0)))",
	};

	for (i = 0; i < (sizeof ewkt / sizeof(char *)); i++) {
		LWGEOM	       *geom_in, *geom_out;
		char	       *in_ewkt;
		char	       *out_ewkt;

		in_ewkt = ewkt[i];
		geom_in = lwgeom_from_wkt(in_ewkt, LW_PARSER_CHECK_NONE);
		geom_out = lwgeom_sfcgal_noop(geom_in);
		if (!geom_out) {
			fprintf(stderr, "\nNull return from lwgeom_sfcgal_noop with wkt:   %s\n", in_ewkt);
			lwgeom_free(geom_in);
			continue;
		}
		out_ewkt = lwgeom_to_ewkt(geom_out);
		if (strcmp(expected_ewkt[i], out_ewkt))
			fprintf(stderr, "\nExp:   %s\nObt:  %s\n", expected_ewkt[i], out_ewkt);
		CU_ASSERT_STRING_EQUAL(expected_ewkt[i], out_ewkt);
		lwfree(out_ewkt);
		lwgeom_free(geom_out);
		lwgeom_free(geom_in);
	}
}


/*
 * * Used by test harness to register the tests in this file.
 */
void		sfcgal_suite_setup(void);
void
sfcgal_suite_setup(void)
{
	CU_pSuite	suite = CU_add_suite("sfcgal", NULL, NULL);
	PG_ADD_TEST(suite, test_sfcgal_noop);
}
