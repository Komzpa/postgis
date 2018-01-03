/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 * Copyright 2008 Paul Ramsey
 * Copyright 2018 Darafei Praliaskouski, me@komzpa.net
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CUnit/Basic.h"

#include "liblwgeom_internal.h"
#include "cu_tester.h"

/*
** Global variables used by tests below
*/

/* Two-point objects */
POINTARRAY *pa21 = NULL;
POINTARRAY *pa22 = NULL;
LWLINE *l21 = NULL;
LWLINE *l22 = NULL;
/* Parsing support */
LWGEOM_PARSER_RESULT parse_result;


/*
** The suite initialization function.
*/
static int init_cg_suite(void)
{
	pa21 = ptarray_construct(0, 0, 2);
	pa22 = ptarray_construct(0, 0, 2);
	l21 = lwline_construct(SRID_UNKNOWN, NULL, pa21);
	l22 = lwline_construct(SRID_UNKNOWN, NULL, pa22);
	return 0;

}

/*
** The suite cleanup function.
** Frees any global objects.
*/
static int clean_cg_suite(void)
{
	if ( l21 ) lwline_free(l21);
	if ( l22 ) lwline_free(l22);
	return 0;
}

/*
** Test left/right side.
*/
static void test_lw_segment_side(void)
{
	int rv = 0;
	POINT2D p1, p2, q;

	/* Vertical line at x=0 */
	p1.x = 0.0;
	p1.y = 0.0;
	p2.x = 0.0;
	p2.y = 1.0;

	/* On the left */
	q.x = -2.0;
	q.y = 1.5;
	rv = lw_segment_side(&p1, &p2, &q);
	//printf("left %g\n",rv);
	CU_ASSERT(rv < 0);

	/* On the right */
	q.x = 2.0;
	rv = lw_segment_side(&p1, &p2, &q);
	//printf("right %g\n",rv);
	CU_ASSERT(rv > 0);

	/* On the line */
	q.x = 0.0;
	rv = lw_segment_side(&p1, &p2, &q);
	//printf("on line %g\n",rv);
	CU_ASSERT_EQUAL(rv, 0);

}

static void test_lw_arc_center(void)
{
/* double lw_arc_center(const POINT2D *p1, const POINT2D *p2, const POINT2D *p3, POINT2D *result); */
	POINT2D c1;
	double d1;
	POINT2D p1, p2, p3;

	p1.x = 2047538.600;
	p1.y = 7268770.435;
	p2.x = 2047538.598;
	p2.y = 7268770.435;
	p3.x = 2047538.596;
	p3.y = 7268770.436;

	d1 = lw_arc_center(&p1, &p2, &p3, &c1);

	CU_ASSERT_DOUBLE_EQUAL(d1, 0.0046097720751, 0.0001);
	CU_ASSERT_DOUBLE_EQUAL(c1.x, 2047538.599, 0.001);
	CU_ASSERT_DOUBLE_EQUAL(c1.y, 7268770.4395, 0.001);

	// printf("lw_arc_center: (%12.12g, %12.12g) %12.12g\n", c1.x, c1.y, d1);
}

/*
** Test crossings side.
*/
static void test_lw_segment_intersects(void)
{

#define setpoint(p, x1, y1) {(p).x = (x1); (p).y = (y1);}

	POINT2D p1, p2, q1, q2;

	/* P: Vertical line at x=0 */
	setpoint(p1, 0.0, 0.0);
	p1.x = 0.0;
	p1.y = 0.0;
	p2.x = 0.0;
	p2.y = 1.0;

	/* Q: Horizontal line crossing left to right */
	q1.x = -0.5;
	q1.y = 0.5;
	q2.x = 0.5;
	q2.y = 0.5;
	CU_ASSERT( lw_segment_intersects(&p1, &p2, &q1, &q2) == SEG_CROSS_RIGHT );

	/* Q: Horizontal line crossing right to left */
	q1.x = 0.5;
	q1.y = 0.5;
	q2.x = -0.5;
	q2.y = 0.5;
	CU_ASSERT( lw_segment_intersects(&p1, &p2, &q1, &q2) == SEG_CROSS_LEFT );

	/* Q: Horizontal line not crossing right to left */
	q1.x = 0.5;
	q1.y = 1.5;
	q2.x = -0.5;
	q2.y = 1.5;
	CU_ASSERT( lw_segment_intersects(&p1, &p2, &q1, &q2) == SEG_NO_INTERSECTION );

	/* Q: Horizontal line crossing at second vertex right to left */
	q1.x = 0.5;
	q1.y = 1.0;
	q2.x = -0.5;
	q2.y = 1.0;
	CU_ASSERT( lw_segment_intersects(&p1, &p2, &q1, &q2) == SEG_NO_INTERSECTION );

	/* Q: Horizontal line crossing at first vertex right to left */
	q1.x = 0.5;
	q1.y = 0.0;
	q2.x = -0.5;
	q2.y = 0.0;
	CU_ASSERT( lw_segment_intersects(&p1, &p2, &q1, &q2) == SEG_CROSS_LEFT );

	/* Q: Diagonal line with large range crossing at first vertex right to left */
	q1.x = 0.5;
	q1.y = 10.0;
	q2.x = -0.5;
	q2.y = -10.0;
	CU_ASSERT( lw_segment_intersects(&p1, &p2, &q1, &q2) == SEG_CROSS_LEFT );

	/* Q: Diagonal line with large range crossing at second vertex right to left */
	q1.x = 0.5;
	q1.y = 11.0;
	q2.x = -0.5;
	q2.y = -9.0;
	CU_ASSERT( lw_segment_intersects(&p1, &p2, &q1, &q2) == SEG_NO_INTERSECTION );

	/* Q: Horizontal touching from left at second vertex*/
	q1.x = -0.5;
	q1.y = 0.5;
	q2.x = 0.0;
	q2.y = 0.5;
	CU_ASSERT( lw_segment_intersects(&p1, &p2, &q1, &q2) == SEG_NO_INTERSECTION );

	/* Q: Horizontal touching from right at first vertex */
	q1.x = 0.0;
	q1.y = 0.5;
	q2.x = 0.5;
	q2.y = 0.5;
	CU_ASSERT( lw_segment_intersects(&p1, &p2, &q1, &q2) == SEG_CROSS_RIGHT );

	/* Q: Horizontal touching from left and far below on second vertex */
	q1.x = -0.5;
	q1.y = -10.5;
	q2.x = 0.0;
	q2.y = 0.5;
	CU_ASSERT( lw_segment_intersects(&p1, &p2, &q1, &q2) == SEG_NO_INTERSECTION );

	/* Q: Horizontal touching from right and far above on second vertex */
	q1.x = 0.5;
	q1.y = 10.5;
	q2.x = 0.0;
	q2.y = 0.5;
	CU_ASSERT( lw_segment_intersects(&p1, &p2, &q1, &q2) == SEG_NO_INTERSECTION );

	/* Q: Co-linear from top */
	q1.x = 0.0;
	q1.y = 10.0;
	q2.x = 0.0;
	q2.y = 0.5;
	CU_ASSERT( lw_segment_intersects(&p1, &p2, &q1, &q2) == SEG_COLINEAR );

	/* Q: Co-linear from bottom */
	q1.x = 0.0;
	q1.y = -10.0;
	q2.x = 0.0;
	q2.y = 0.5;
	CU_ASSERT( lw_segment_intersects(&p1, &p2, &q1, &q2) == SEG_COLINEAR );

	/* Q: Co-linear contained */
	q1.x = 0.0;
	q1.y = 0.4;
	q2.x = 0.0;
	q2.y = 0.5;
	CU_ASSERT( lw_segment_intersects(&p1, &p2, &q1, &q2) == SEG_COLINEAR );

	/* Q: Horizontal touching at end point from left */
	q1.x = -0.5;
	q1.y = 1.0;
	q2.x = 0.0;
	q2.y = 1.0;
	CU_ASSERT( lw_segment_intersects(&p1, &p2, &q1, &q2) == SEG_NO_INTERSECTION );

	/* Q: Horizontal touching at end point from right */
	q1.x = 0.0;
	q1.y = 1.0;
	q2.x = 0.0;
	q2.y = 0.5;
	CU_ASSERT( lw_segment_intersects(&p1, &p2, &q1, &q2) == SEG_COLINEAR );

	/* Q: Horizontal touching at start point from left */
	q1.x = 0.0;
	q1.y = 0.0;
	q2.x = -0.5;
	q2.y = 0.0;
	CU_ASSERT( lw_segment_intersects(&p1, &p2, &q1, &q2) == SEG_CROSS_LEFT );

	/* Q: Horizontal touching at start point from right */
	q1.x = 0.0;
	q1.y = 0.0;
	q2.x = 0.5;
	q2.y = 0.0;
	CU_ASSERT( lw_segment_intersects(&p1, &p2, &q1, &q2) == SEG_CROSS_RIGHT );

}

static void test_lwline_crossing_short_lines(void)
{

	POINT4D p;

	/*
	** Simple test, two two-point lines
	*/

	/* Vertical line from 0,0 to 1,1 */
	p.x = 0.0;
	p.y = 0.0;
	ptarray_set_point4d(pa21, 0, &p);
	p.y = 1.0;
	ptarray_set_point4d(pa21, 1, &p);

	/* Horizontal, crossing mid-segment */
	p.x = -0.5;
	p.y = 0.5;
	ptarray_set_point4d(pa22, 0, &p);
	p.x = 0.5;
	ptarray_set_point4d(pa22, 1, &p);

	CU_ASSERT( lwline_crossing_direction(l21, l22) == LINE_CROSS_RIGHT );

	/* Horizontal, crossing at top end vertex (end crossings don't count) */
	p.x = -0.5;
	p.y = 1.0;
	ptarray_set_point4d(pa22, 0, &p);
	p.x = 0.5;
	ptarray_set_point4d(pa22, 1, &p);

	CU_ASSERT( lwline_crossing_direction(l21, l22) == LINE_NO_CROSS );

	/* Horizontal, crossing at bottom end vertex */
	p.x = -0.5;
	p.y = 0.0;
	ptarray_set_point4d(pa22, 0, &p);
	p.x = 0.5;
	ptarray_set_point4d(pa22, 1, &p);

	CU_ASSERT( lwline_crossing_direction(l21, l22) == LINE_CROSS_RIGHT );

	/* Horizontal, no crossing */
	p.x = -0.5;
	p.y = 2.0;
	ptarray_set_point4d(pa22, 0, &p);
	p.x = 0.5;
	ptarray_set_point4d(pa22, 1, &p);

	CU_ASSERT( lwline_crossing_direction(l21, l22) == LINE_NO_CROSS );

	/* Vertical, no crossing */
	p.x = -0.5;
	p.y = 0.0;
	ptarray_set_point4d(pa22, 0, &p);
	p.y = 1.0;
	ptarray_set_point4d(pa22, 1, &p);

	CU_ASSERT( lwline_crossing_direction(l21, l22) == LINE_NO_CROSS );

}

static void test_lwline_crossing_long_lines(void)
{
	LWLINE *l51;
	LWLINE *l52;
	/*
	** More complex test, longer lines and multiple crossings
	*/
	/* Vertical line with vertices at y integers */
	l51 = (LWLINE*)lwgeom_from_wkt("LINESTRING(0 0, 0 1, 0 2, 0 3, 0 4)", LW_PARSER_CHECK_NONE);

	/* Two crossings at segment midpoints */
	l52 = (LWLINE*)lwgeom_from_wkt("LINESTRING(1 1, -1 1.5, 1 3, 1 4, 1 5)", LW_PARSER_CHECK_NONE);
	CU_ASSERT( lwline_crossing_direction(l51, l52) == LINE_MULTICROSS_END_SAME_FIRST_LEFT );
	lwline_free(l52);

	/* One crossing at interior vertex */
	l52 = (LWLINE*)lwgeom_from_wkt("LINESTRING(1 1, 0 1, -1 1, -1 2, -1 3)", LW_PARSER_CHECK_NONE);
	CU_ASSERT( lwline_crossing_direction(l51, l52) == LINE_CROSS_LEFT );
	lwline_free(l52);

	/* Two crossings at interior vertices */
	l52 = (LWLINE*)lwgeom_from_wkt("LINESTRING(1 1, 0 1, -1 1, 0 3, 1 3)", LW_PARSER_CHECK_NONE);
	CU_ASSERT( lwline_crossing_direction(l51, l52) == LINE_MULTICROSS_END_SAME_FIRST_LEFT );
	lwline_free(l52);

	/* Two crossings, one at the first vertex on at interior vertex */
	l52 = (LWLINE*)lwgeom_from_wkt("LINESTRING(1 0, 0 0, -1 1, 0 3, 1 3)", LW_PARSER_CHECK_NONE);
	CU_ASSERT( lwline_crossing_direction(l51, l52) == LINE_MULTICROSS_END_SAME_FIRST_LEFT );
	lwline_free(l52);

	/* Two crossings, one at the first vertex on the next interior vertex */
	l52 = (LWLINE*)lwgeom_from_wkt("LINESTRING(1 0, 0 0, -1 1, 0 1, 1 2)", LW_PARSER_CHECK_NONE);
	CU_ASSERT( lwline_crossing_direction(l51, l52) == LINE_MULTICROSS_END_SAME_FIRST_LEFT );
	lwline_free(l52);

	/* Three crossings, two at midpoints, one at vertex */
	l52 = (LWLINE*)lwgeom_from_wkt("LINESTRING(0.5 1, -1 0.5, 1 2, -1 2, -1 3)", LW_PARSER_CHECK_NONE);
	CU_ASSERT( lwline_crossing_direction(l51, l52) == LINE_MULTICROSS_END_LEFT );
	lwline_free(l52);

	/* One mid-point co-linear crossing */
	l52 = (LWLINE*)lwgeom_from_wkt("LINESTRING(1 1, 0 1.5, 0 2.5, -1 3, -1 4)", LW_PARSER_CHECK_NONE);
	CU_ASSERT( lwline_crossing_direction(l51, l52) == LINE_CROSS_LEFT );
	lwline_free(l52);

	/* One on-vertices co-linear crossing */
	l52 = (LWLINE*)lwgeom_from_wkt("LINESTRING(1 1, 0 1, 0 2, -1 4, -1 4)", LW_PARSER_CHECK_NONE);
	CU_ASSERT( lwline_crossing_direction(l51, l52) == LINE_CROSS_LEFT );
	lwline_free(l52);

	/* No crossing, but end on a co-linearity. */
	l52 = (LWLINE*)lwgeom_from_wkt("LINESTRING(1 1, 1 2, 1 3, 0 3, 0 4)", LW_PARSER_CHECK_NONE);
	CU_ASSERT( lwline_crossing_direction(l51, l52) == LINE_NO_CROSS );
	lwline_free(l52);

	lwline_free(l51);

}


static void test_lwline_crossing_bugs(void)
{
	LWLINE *l1;
	LWLINE *l2;

	l1 = (LWLINE*)lwgeom_from_wkt("LINESTRING(2.99 90.16,71 74,20 140,171 154)", LW_PARSER_CHECK_NONE);
	l2 = (LWLINE*)lwgeom_from_wkt("LINESTRING(25 169,89 114,40 70,86 43)", LW_PARSER_CHECK_NONE);

	CU_ASSERT( lwline_crossing_direction(l1, l2) == LINE_MULTICROSS_END_RIGHT );
	lwline_free(l1);
	lwline_free(l2);

}

static void test_lwpoint_set_ordinate(void)
{
	POINT4D p;

	p.x = 0.0;
	p.y = 0.0;
	p.z = 0.0;
	p.m = 0.0;

	lwpoint_set_ordinate(&p, 'X', 1.5);
	CU_ASSERT_EQUAL( p.x, 1.5 );

	lwpoint_set_ordinate(&p, 'M', 2.5);
	CU_ASSERT_EQUAL( p.m, 2.5 );

	lwpoint_set_ordinate(&p, 'Z', 3.5);
	CU_ASSERT_EQUAL( p.z, 3.5 );

}

static void test_lwpoint_get_ordinate(void)
{
	POINT4D p;

	p.x = 10.0;
	p.y = 20.0;
	p.z = 30.0;
	p.m = 40.0;

	CU_ASSERT_EQUAL( lwpoint_get_ordinate(&p, 'X'), 10.0 );
	CU_ASSERT_EQUAL( lwpoint_get_ordinate(&p, 'Y'), 20.0 );
	CU_ASSERT_EQUAL( lwpoint_get_ordinate(&p, 'Z'), 30.0 );
	CU_ASSERT_EQUAL( lwpoint_get_ordinate(&p, 'M'), 40.0 );

}

static void test_point_interpolate(void)
{
	POINT4D p, q, r;
	int rv = 0;

	p.x = 10.0;
	p.y = 20.0;
	p.z = 30.0;
	p.m = 40.0;

	q.x = 20.0;
	q.y = 30.0;
	q.z = 40.0;
	q.m = 50.0;

	rv = point_interpolate(&p, &q, &r, 1, 1, 'Z', 35.0);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_EQUAL( r.x, 15.0);

	rv = point_interpolate(&p, &q, &r, 1, 1, 'M', 41.0);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_EQUAL( r.y, 21.0);

	rv = point_interpolate(&p, &q, &r, 1, 1, 'M', 50.0);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_EQUAL( r.y, 30.0);

	rv = point_interpolate(&p, &q, &r, 1, 1, 'M', 40.0);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_EQUAL( r.y, 20.0);

}

static void test_lwline_interpolate_points(void)
{
	LWLINE* line = lwgeom_as_lwline(lwgeom_from_wkt("LINESTRING ZM (0 0 3 1, 1 1 2 2, 10 10 4 3)", LW_PARSER_CHECK_NONE));
	LWLINE* line2d = lwgeom_as_lwline(lwgeom_from_wkt("LINESTRING (1 1, 3 7, 9 12)", LW_PARSER_CHECK_NONE));
	LWLINE* empty_line = lwgeom_as_lwline(lwgeom_from_wkt("LINESTRING EMPTY", LW_PARSER_CHECK_NONE));

	POINTARRAY* rpa;
	POINT4D pta;
	POINT4D ptb;
	double eps = 1e-10;

	/* Empty line gives empty point */
	rpa = lwline_interpolate_points(empty_line, 0.5, LW_TRUE);
	ASSERT_INT_EQUAL(rpa->npoints, 0);
	ptarray_free(rpa);

	/* Get first endpoint when fraction = 0 */
	rpa = lwline_interpolate_points(line, 0, LW_TRUE);
	ASSERT_INT_EQUAL(rpa->npoints, 1);
	pta = getPoint4d(line->points, 0);
	ptb = getPoint4d(rpa, 0);
	ASSERT_POINT4D_EQUAL(pta, ptb, eps);
	ptarray_free(rpa);

	/* Get last endpoint when fraction = 0 */
	rpa = lwline_interpolate_points(line, 1, LW_TRUE);
	ASSERT_INT_EQUAL(rpa->npoints, 1);
	pta = getPoint4d(line->points, line->points->npoints - 1);
	ptb = getPoint4d(rpa, 0);
	ASSERT_POINT4D_EQUAL(pta, ptb, eps);
	ptarray_free(rpa);

    /* Interpolate a single point */
    /* First vertex is at 10% */
	rpa = lwline_interpolate_points(line, 0.1, LW_FALSE);
	ASSERT_INT_EQUAL(rpa->npoints, 1);
	pta = getPoint4d(line->points, 1);
	ptb = getPoint4d(rpa, 0);
	ASSERT_POINT4D_EQUAL(pta, ptb, eps);
	ptarray_free(rpa);

	/* 5% is halfway to first vertex */
	rpa = lwline_interpolate_points(line, 0.05, LW_FALSE);
	ASSERT_INT_EQUAL(rpa->npoints, 1);
	pta.x = 0.5;
	pta.y = 0.5;
	pta.m = 1.5;
	pta.z = 2.5;
	ptb = getPoint4d(rpa, 0);
	ASSERT_POINT4D_EQUAL(pta, ptb, eps);
	ptarray_free(rpa);

    /* Now repeat points */
	rpa = lwline_interpolate_points(line, 0.4, LW_TRUE);
	ASSERT_INT_EQUAL(rpa->npoints, 2);
	pta.x = 4;
	pta.y = 4;
	ptb = getPoint4d(rpa, 0);
	ASSERT_POINT2D_EQUAL(pta, ptb, eps);

	pta.x = 8;
	pta.y = 8;
	ptb = getPoint4d(rpa, 1);
	ASSERT_POINT2D_EQUAL(pta, ptb, eps);
	ptarray_free(rpa);

	/* Make sure it works on 2D lines */
	rpa = lwline_interpolate_points(line2d, 0.4, LW_TRUE);
	ASSERT_INT_EQUAL(rpa->npoints, 2);
	CU_ASSERT_FALSE(ptarray_has_z(rpa));
	CU_ASSERT_FALSE(ptarray_has_m(rpa));
	ptarray_free(rpa);

	lwgeom_free(lwline_as_lwgeom(line));
	lwgeom_free(lwline_as_lwgeom(line2d));
	lwgeom_free(lwline_as_lwgeom(empty_line));
}

static void test_lwline_clip(void)
{
	LWCOLLECTION *c;
	LWLINE *line = NULL;
	LWLINE *l51 = NULL;
	char *ewkt;

	/* Vertical line with vertices at y integers */
	l51 = (LWLINE*)lwgeom_from_wkt("LINESTRING(0 0, 0 1, 0 2, 0 3, 0 4)", LW_PARSER_CHECK_NONE);

	/* Clip in the middle, mid-range. */
	c = lwline_clip_to_ordinate_range(l51, 'Y', 1.5, 2.5);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c);
	//printf("c = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "MULTILINESTRING((0 1.5,0 2,0 2.5))");
	lwfree(ewkt);
	lwcollection_free(c);

	/* Clip off the top. */
	c = lwline_clip_to_ordinate_range(l51, 'Y', 3.5, 5.5);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c);
	//printf("c = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "MULTILINESTRING((0 3.5,0 4))");
	lwfree(ewkt);
	lwcollection_free(c);

	/* Clip off the bottom. */
	c = lwline_clip_to_ordinate_range(l51, 'Y', -1.5, 2.5);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c);
	//printf("c = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "MULTILINESTRING((0 0,0 1,0 2,0 2.5))" );
	lwfree(ewkt);
	lwcollection_free(c);

	/* Range holds entire object. */
	c = lwline_clip_to_ordinate_range(l51, 'Y', -1.5, 5.5);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c);
	//printf("c = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "MULTILINESTRING((0 0,0 1,0 2,0 3,0 4))" );
	lwfree(ewkt);
	lwcollection_free(c);

	/* Clip on vertices. */
	c = lwline_clip_to_ordinate_range(l51, 'Y', 1.0, 2.0);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c);
	//printf("c = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "MULTILINESTRING((0 1,0 2))" );
	lwfree(ewkt);
	lwcollection_free(c);

	/* Clip on vertices off the bottom. */
	c = lwline_clip_to_ordinate_range(l51, 'Y', -1.0, 2.0);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c);
	//printf("c = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "MULTILINESTRING((0 0,0 1,0 2))" );
	lwfree(ewkt);
	lwcollection_free(c);

	/* Clip on top. */
	c = lwline_clip_to_ordinate_range(l51, 'Y', -1.0, 0.0);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c);
	//printf("c = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "GEOMETRYCOLLECTION(POINT(0 0))" );
	lwfree(ewkt);
	lwcollection_free(c);

	/* ST_LocateBetweenElevations(ST_GeomFromEWKT('LINESTRING(1 2 3, 4 5 6, 6 6 6, 1 1 1)'), 1, 2)) */
	line = (LWLINE*)lwgeom_from_wkt("LINESTRING(1 2 3, 4 5 6, 6 6 6, 1 1 1)", LW_PARSER_CHECK_NONE);
	c = lwline_clip_to_ordinate_range(line, 'Z', 1.0, 2.0);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c);
	CU_ASSERT_STRING_EQUAL(ewkt, "MULTILINESTRING((2 2 2,1 1 1))" );
	lwfree(ewkt);
	lwcollection_free(c);
	lwline_free(line);

	/* ST_LocateBetweenElevations('LINESTRING(1 2 3, 4 5 6, 6 6 6, 1 1 1)', 1, 2)) */
	line = (LWLINE*)lwgeom_from_wkt("LINESTRING(1 2 3, 4 5 6, 6 6 6, 1 1 1)", LW_PARSER_CHECK_NONE);
	c = lwline_clip_to_ordinate_range(line, 'Z', 1.0, 2.0);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c);
	//printf("a = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "MULTILINESTRING((2 2 2,1 1 1))" );
	lwfree(ewkt);
	lwcollection_free(c);
	lwline_free(line);

	/* ST_LocateBetweenElevations('LINESTRING(1 2 3, 4 5 6, 6 6 6, 1 1 1)', 1, 1)) */
	line = (LWLINE*)lwgeom_from_wkt("LINESTRING(1 2 3, 4 5 6, 6 6 6, 1 1 1)", LW_PARSER_CHECK_NONE);
	c = lwline_clip_to_ordinate_range(line, 'Z', 1.0, 1.0);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c);
	//printf("b = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "GEOMETRYCOLLECTION(POINT(1 1 1))" );
	lwfree(ewkt);
	lwcollection_free(c);
	lwline_free(line);

	/* ST_LocateBetweenElevations('LINESTRING(1 1 1, 1 2 2)', 1,1) */
	line = (LWLINE*)lwgeom_from_wkt("LINESTRING(1 1 1, 1 2 2)", LW_PARSER_CHECK_NONE);
	c = lwline_clip_to_ordinate_range(line, 'Z', 1.0, 1.0);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c);
	//printf("c = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "GEOMETRYCOLLECTION(POINT(1 1 1))" );
	lwfree(ewkt);
	lwcollection_free(c);
	lwline_free(line);

	lwline_free(l51);

}

static void test_lwmline_clip(void)
{
	LWCOLLECTION *c;
	char *ewkt;
	LWMLINE *mline = NULL;
	LWLINE *line = NULL;

	/*
	** Set up the input line. Trivial one-member case.
	*/
	mline = (LWMLINE*)lwgeom_from_wkt("MULTILINESTRING((0 0,0 1,0 2,0 3,0 4))", LW_PARSER_CHECK_NONE);

	/* Clip in the middle, mid-range. */
	c = lwmline_clip_to_ordinate_range(mline, 'Y', 1.5, 2.5);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c);
	//printf("c = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "MULTILINESTRING((0 1.5,0 2,0 2.5))");
	lwfree(ewkt);
	lwcollection_free(c);

	lwmline_free(mline);

	/*
	** Set up the input line. Two-member case.
	*/
	mline = (LWMLINE*)lwgeom_from_wkt("MULTILINESTRING((1 0,1 1,1 2,1 3,1 4), (0 0,0 1,0 2,0 3,0 4))", LW_PARSER_CHECK_NONE);

	/* Clip off the top. */
	c = lwmline_clip_to_ordinate_range(mline, 'Y', 3.5, 5.5);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c);
	//printf("c = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "MULTILINESTRING((1 3.5,1 4),(0 3.5,0 4))");
	lwfree(ewkt);
	lwcollection_free(c);

	lwmline_free(mline);

	/*
	** Set up staggered input line to create multi-type output.
	*/
	mline = (LWMLINE*)lwgeom_from_wkt("MULTILINESTRING((1 0,1 -1,1 -2,1 -3,1 -4), (0 0,0 1,0 2,0 3,0 4))", LW_PARSER_CHECK_NONE);

	/* Clip from 0 upwards.. */
	c = lwmline_clip_to_ordinate_range(mline, 'Y', 0.0, 2.5);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c);
	//printf("c = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "GEOMETRYCOLLECTION(POINT(1 0),LINESTRING(0 0,0 1,0 2,0 2.5))");
	lwfree(ewkt);
	lwcollection_free(c);

	lwmline_free(mline);

	/*
	** Set up input line from MAC
	*/
	line = (LWLINE*)lwgeom_from_wkt("LINESTRING(0 0 0 0,1 1 1 1,2 2 2 2,3 3 3 3,4 4 4 4,3 3 3 5,2 2 2 6,1 1 1 7,0 0 0 8)", LW_PARSER_CHECK_NONE);

	/* Clip from 3 to 3.5 */
	c = lwline_clip_to_ordinate_range(line, 'Z', 3.0, 3.5);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c);
	//printf("c = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "MULTILINESTRING((3 3 3 3,3.5 3.5 3.5 3.5),(3.5 3.5 3.5 4.5,3 3 3 5))");
	lwfree(ewkt);
	lwcollection_free(c);

	/* Clip from 2 to 3.5 */
	c = lwline_clip_to_ordinate_range(line, 'Z', 2.0, 3.5);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c);
	//printf("c = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "MULTILINESTRING((2 2 2 2,3 3 3 3,3.5 3.5 3.5 3.5),(3.5 3.5 3.5 4.5,3 3 3 5,2 2 2 6))");
	lwfree(ewkt);
	lwcollection_free(c);

	/* Clip from 3 to 4 */
	c = lwline_clip_to_ordinate_range(line, 'Z', 3.0, 4.0);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c);
	//printf("c = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "MULTILINESTRING((3 3 3 3,4 4 4 4,3 3 3 5))");
	lwfree(ewkt);
	lwcollection_free(c);

	/* Clip from 2 to 3 */
	c = lwline_clip_to_ordinate_range(line, 'Z', 2.0, 3.0);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c);
	//printf("c = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "MULTILINESTRING((2 2 2 2,3 3 3 3),(3 3 3 5,2 2 2 6))");
	lwfree(ewkt);
	lwcollection_free(c);


	lwline_free(line);

}



static void test_lwline_clip_big(void)
{
	POINTARRAY *pa = ptarray_construct(1, 0, 3);
	LWLINE *line = lwline_construct(SRID_UNKNOWN, NULL, pa);
	LWCOLLECTION *c;
	char *ewkt;
	POINT4D p;

	p.x = 0.0;
	p.y = 0.0;
	p.z = 0.0;
	ptarray_set_point4d(pa, 0, &p);

	p.x = 1.0;
	p.y = 1.0;
	p.z = 1.0;
	ptarray_set_point4d(pa, 1, &p);

	p.x = 2.0;
	p.y = 2.0;
	p.z = 2.0;
	ptarray_set_point4d(pa, 2, &p);

	c = lwline_clip_to_ordinate_range(line, 'Z', 0.5, 1.5);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c);
	//printf("c = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "MULTILINESTRING((0.5 0.5 0.5,1 1 1,1.5 1.5 1.5))" );

	lwfree(ewkt);
	lwcollection_free(c);
	lwline_free(line);
}

static void test_geohash_precision(void)
{
	GBOX bbox;
	GBOX bounds;
	int precision = 0;
	gbox_init(&bbox);
	gbox_init(&bounds);

	bbox.xmin = 23.0;
	bbox.xmax = 23.0;
	bbox.ymin = 25.2;
	bbox.ymax = 25.2;
	precision = lwgeom_geohash_precision(bbox, &bounds);
	//printf("\nprecision %d\n",precision);
	CU_ASSERT_EQUAL(precision, 20);

	bbox.xmin = 23.0;
	bbox.ymin = 23.0;
	bbox.xmax = 23.1;
	bbox.ymax = 23.1;
	precision = lwgeom_geohash_precision(bbox, &bounds);
	//printf("precision %d\n",precision);
	CU_ASSERT_EQUAL(precision, 3);

	bbox.xmin = 23.0;
	bbox.ymin = 23.0;
	bbox.xmax = 23.0001;
	bbox.ymax = 23.0001;
	precision = lwgeom_geohash_precision(bbox, &bounds);
	//printf("precision %d\n",precision);
	CU_ASSERT_EQUAL(precision, 7);

}

static void test_geohash_point(void)
{
	char *geohash;

	geohash = geohash_point(0, 0, 16);
	//printf("\ngeohash %s\n",geohash);
	CU_ASSERT_STRING_EQUAL(geohash, "s000000000000000");
	lwfree(geohash);

	geohash = geohash_point(90, 0, 16);
	//printf("\ngeohash %s\n",geohash);
	CU_ASSERT_STRING_EQUAL(geohash, "w000000000000000");
	lwfree(geohash);

	geohash = geohash_point(20.012345, -20.012345, 15);
	//printf("\ngeohash %s\n",geohash);
	CU_ASSERT_STRING_EQUAL(geohash, "kkqnpkue9ktbpe5");
	lwfree(geohash);

}

static void test_geohash(void)
{
	LWPOINT *lwpoint = NULL;
	LWLINE *lwline = NULL;
	LWMLINE *lwmline = NULL;
	char *geohash = NULL;

	lwpoint = (LWPOINT*)lwgeom_from_wkt("POINT(23.0 25.2)", LW_PARSER_CHECK_NONE);
	geohash = lwgeom_geohash((LWGEOM*)lwpoint,0);
	//printf("\ngeohash %s\n",geohash);
	CU_ASSERT_STRING_EQUAL(geohash, "ss2r77s0du7p2ewb8hmx");
	lwpoint_free(lwpoint);
	lwfree(geohash);

	lwpoint = (LWPOINT*)lwgeom_from_wkt("POINT(23.0 25.2 2.0)", LW_PARSER_CHECK_NONE);
	geohash = lwgeom_geohash((LWGEOM*)lwpoint,0);
	//printf("geohash %s\n",geohash);
	CU_ASSERT_STRING_EQUAL(geohash, "ss2r77s0du7p2ewb8hmx");
	lwpoint_free(lwpoint);
	lwfree(geohash);

	lwline = (LWLINE*)lwgeom_from_wkt("LINESTRING(23.0 23.0,23.1 23.1)", LW_PARSER_CHECK_NONE);
	geohash = lwgeom_geohash((LWGEOM*)lwline,0);
	//printf("geohash %s\n",geohash);
	CU_ASSERT_STRING_EQUAL(geohash, "ss0");
	lwline_free(lwline);
	lwfree(geohash);

	lwline = (LWLINE*)lwgeom_from_wkt("LINESTRING(23.0 23.0,23.001 23.001)", LW_PARSER_CHECK_NONE);
	geohash = lwgeom_geohash((LWGEOM*)lwline,0);
	//printf("geohash %s\n",geohash);
	CU_ASSERT_STRING_EQUAL(geohash, "ss06g7h");
	lwline_free(lwline);
	lwfree(geohash);

	lwmline = (LWMLINE*)lwgeom_from_wkt("MULTILINESTRING((23.0 23.0,23.1 23.1),(23.0 23.0,23.1 23.1))", LW_PARSER_CHECK_NONE);
	geohash = lwgeom_geohash((LWGEOM*)lwmline,0);
	//printf("geohash %s\n",geohash);
	CU_ASSERT_STRING_EQUAL(geohash, "ss0");
	lwmline_free(lwmline);
	lwfree(geohash);
}

static void test_isclosed(void)
{
	LWGEOM *geom;

	/* LINESTRING */

	/* Not Closed on 2D */
	geom = lwgeom_from_wkt("LINESTRING(1 2,3 4)", LW_PARSER_CHECK_NONE);
	CU_ASSERT(!lwline_is_closed((LWLINE *) geom));
	lwgeom_free(geom);

	/* Closed on 2D */
	geom = lwgeom_from_wkt("LINESTRING(1 2,3 4,1 2)", LW_PARSER_CHECK_NONE);
	CU_ASSERT(lwline_is_closed((LWLINE *) geom));
	lwgeom_free(geom);

	/* Not closed on 3D */
	geom = lwgeom_from_wkt("LINESTRING(1 2 3,4 5 6)", LW_PARSER_CHECK_NONE);
	CU_ASSERT(!lwline_is_closed((LWLINE *) geom));
	lwgeom_free(geom);

	/* Closed on 3D */
	geom = lwgeom_from_wkt("LINESTRING(1 2 3,4 5 6,1 2 3)", LW_PARSER_CHECK_NONE);
	CU_ASSERT(lwline_is_closed((LWLINE *) geom));
	lwgeom_free(geom);

	/* Closed on 4D, even if M is not the same */
	geom = lwgeom_from_wkt("LINESTRING(1 2 3 4,5 6 7 8,1 2 3 0)", LW_PARSER_CHECK_NONE);
	CU_ASSERT(lwline_is_closed((LWLINE *) geom));
	lwgeom_free(geom);


	/* CIRCULARSTRING */

	/* Not Closed on 2D */
	geom = lwgeom_from_wkt("CIRCULARSTRING(1 2,3 4,5 6)", LW_PARSER_CHECK_NONE);
	CU_ASSERT(!lwcircstring_is_closed((LWCIRCSTRING *) geom));
	lwgeom_free(geom);

	/* Closed on 2D */
	geom = lwgeom_from_wkt("CIRCULARSTRING(1 2,3 4,1 2)", LW_PARSER_CHECK_NONE);
	CU_ASSERT(lwcircstring_is_closed((LWCIRCSTRING *) geom));
	lwgeom_free(geom);

	/* Not closed on 3D */
	geom = lwgeom_from_wkt("CIRCULARSTRING(1 2 3,4 5 6,7 8 9)", LW_PARSER_CHECK_NONE);
	CU_ASSERT(!lwcircstring_is_closed((LWCIRCSTRING *) geom));
	lwgeom_free(geom);

	/* Closed on 3D */
	geom = lwgeom_from_wkt("CIRCULARSTRING(1 2 3,4 5 6,1 2 3)", LW_PARSER_CHECK_NONE);
	CU_ASSERT(lwcircstring_is_closed((LWCIRCSTRING *) geom));
	lwgeom_free(geom);

	/* Closed on 4D, even if M is not the same */
	geom = lwgeom_from_wkt("CIRCULARSTRING(1 2 3 4,5 6 7 8,1 2 3 0)", LW_PARSER_CHECK_NONE);
	CU_ASSERT(lwcircstring_is_closed((LWCIRCSTRING *) geom));
	lwgeom_free(geom);


	/* COMPOUNDCURVE */

	/* Not Closed on 2D */
	geom = lwgeom_from_wkt("COMPOUNDCURVE(CIRCULARSTRING(1 2,3 4,1 2),(1 2,7 8,5 6))", LW_PARSER_CHECK_NONE);
	CU_ASSERT(!lwcompound_is_closed((LWCOMPOUND *) geom));
	lwgeom_free(geom);

	geom = lwgeom_from_wkt("COMPOUNDCURVE((1 2,3 4,1 2),CIRCULARSTRING(1 2,7 8,5 6))", LW_PARSER_CHECK_NONE);
	CU_ASSERT(!lwcompound_is_closed((LWCOMPOUND *) geom));
	lwgeom_free(geom);

	/* Closed on 2D */
	geom = lwgeom_from_wkt("COMPOUNDCURVE(CIRCULARSTRING(1 2,3 4,5 6), (5 6,7 8,1 2))", LW_PARSER_CHECK_NONE);
	CU_ASSERT(lwcompound_is_closed((LWCOMPOUND *) geom));
	lwgeom_free(geom);

	geom = lwgeom_from_wkt("COMPOUNDCURVE((1 2,3 4,5 6),CIRCULARSTRING(5 6,7 8,1 2))", LW_PARSER_CHECK_NONE);
	CU_ASSERT(lwcompound_is_closed((LWCOMPOUND *) geom));
	lwgeom_free(geom);

	/* Not Closed on 3D */
	geom = lwgeom_from_wkt("COMPOUNDCURVE(CIRCULARSTRING(1 2 3,4 5 6,1 2 3),(1 2 3,7 8 9,10 11 12))", LW_PARSER_CHECK_NONE);
	CU_ASSERT(!lwcompound_is_closed((LWCOMPOUND *) geom));
	lwgeom_free(geom);

	geom = lwgeom_from_wkt("COMPOUNDCURVE((1 2 3,4 5 6,1 2 3),CIRCULARSTRING(1 2 3,7 8 9,10 11 12))", LW_PARSER_CHECK_NONE);
	CU_ASSERT(!lwcompound_is_closed((LWCOMPOUND *) geom));
	lwgeom_free(geom);

	/* Closed on 3D */
	geom = lwgeom_from_wkt("COMPOUNDCURVE(CIRCULARSTRING(1 2 3,4 5 6,7 8 9),(7 8 9,10 11 12,1 2 3))", LW_PARSER_CHECK_NONE);
	CU_ASSERT(lwcompound_is_closed((LWCOMPOUND *) geom));
	lwgeom_free(geom);

	geom = lwgeom_from_wkt("COMPOUNDCURVE((1 2 3,4 5 6,7 8 9),CIRCULARSTRING(7 8 9,10 11 12,1 2 3))", LW_PARSER_CHECK_NONE);
	CU_ASSERT(lwcompound_is_closed((LWCOMPOUND *) geom));
	lwgeom_free(geom);

	/* Closed on 4D, even if M is not the same */
	geom = lwgeom_from_wkt("COMPOUNDCURVE((1 2 3 4,5 6 7 8,9 10 11 12),CIRCULARSTRING(9 10 11 12,13 14 15 16,1 2 3 0))", LW_PARSER_CHECK_NONE);
	CU_ASSERT(lwcompound_is_closed((LWCOMPOUND *) geom));
	lwgeom_free(geom);
}


static void test_geohash_point_as_int(void)
{
	unsigned int gh;
	POINT2D p;
	unsigned long long rs;

	p.x = 50; p.y = 35;
	gh = geohash_point_as_int(&p);
	rs = 3440103613;
	CU_ASSERT_EQUAL(gh, rs);
	p.x = 140; p.y = 45;
	gh = geohash_point_as_int(&p);
	rs = 3982480893;
	CU_ASSERT_EQUAL(gh, rs);
	p.x = 140; p.y = 55;
	gh = geohash_point_as_int(&p);
	rs = 4166944232;
	CU_ASSERT_EQUAL(gh, rs);
}

static void test_lwgeom_remove_repeated_points(void)
{
	LWGEOM *g;
	char *ewkt;

	g = lwgeom_from_wkt("MULTIPOINT(0 0, 10 0, 10 10, 10 10, 0 10, 0 10, 0 10, 0 0, 0 0, 0 0, 5 5, 0 0, 5 5)", LW_PARSER_CHECK_NONE);
	lwgeom_remove_repeated_points_in_place(g, 1);
	ewkt = lwgeom_to_ewkt(g);
	CU_ASSERT_STRING_EQUAL(ewkt, "MULTIPOINT(0 0,10 0,10 10,0 10,5 5)");
	// printf("%s\n", ewkt);
	lwgeom_free(g);
	lwfree(ewkt);

	g = lwgeom_from_wkt("LINESTRING(1612830.15445 4841287.12672,1612830.15824 4841287.12674,1612829.98813 4841274.56198)", LW_PARSER_CHECK_NONE);
	lwgeom_remove_repeated_points_in_place(g, 0.01);
	ewkt = lwgeom_to_ewkt(g);
	CU_ASSERT_STRING_EQUAL(ewkt, "LINESTRING(1612830.15445 4841287.12672,1612829.98813 4841274.56198)");
	// printf("%s\n", ewkt);
	lwgeom_free(g);
	lwfree(ewkt);


	g = lwgeom_from_wkt("MULTIPOINT(0 0,10 0,10 10,10 10,0 10,0 10,0 10,0 0,0 0,0 0,5 5,5 5,5 8,8 8,8 8,8 8,8 5,8 5,5 5,5 5,5 5,5 5,5 5,50 50,50 50,50 50,50 60,50 60,50 60,60 60,60 50,60 50,50 50,55 55,55 58,58 58,58 55,58 55,55 55)", LW_PARSER_CHECK_NONE);
	lwgeom_remove_repeated_points_in_place(g, 1);
	ewkt = lwgeom_to_ewkt(g);
	CU_ASSERT_STRING_EQUAL(ewkt, "MULTIPOINT(0 0,10 0,10 10,0 10,5 5,5 8,8 8,8 5,50 50,50 60,60 60,60 50,55 55,55 58,58 58,58 55)");
	// printf("%s\n", ewkt);
	lwgeom_free(g);
	lwfree(ewkt);


}

static void test_lwgeom_simplify(void)
{
	LWGEOM *l;
	LWGEOM *g;
	char *ewkt;

	/* Simplify but only so far... */
	g = lwgeom_from_wkt("LINESTRING(0 0, 1 0, 1 1, 0 1, 0 0)", LW_PARSER_CHECK_NONE);
	l = lwgeom_simplify(g, 10, LW_TRUE);
	ewkt = lwgeom_to_ewkt(l);
	CU_ASSERT_STRING_EQUAL(ewkt, "LINESTRING(0 0,0 0)");
	lwgeom_free(g);
	lwgeom_free(l);
	lwfree(ewkt);

	/* Simplify but only so far... */
	g = lwgeom_from_wkt("POLYGON((0 0, 1 0, 1 1, 0 1, 0 0))", LW_PARSER_CHECK_NONE);
	l = lwgeom_simplify(g, 10, LW_TRUE);
	ewkt = lwgeom_to_ewkt(l);
	CU_ASSERT_STRING_EQUAL(ewkt, "POLYGON((0 0,1 0,1 1,0 0))");
	lwgeom_free(g);
	lwgeom_free(l);
	lwfree(ewkt);

	/* Simplify and collapse */
	g = lwgeom_from_wkt("LINESTRING(0 0, 1 0, 1 1, 0 1, 0 0)", LW_PARSER_CHECK_NONE);
	l = lwgeom_simplify(g, 10, LW_FALSE);
	CU_ASSERT_EQUAL(l, NULL);
	lwgeom_free(g);
	lwgeom_free(l);

	/* Simplify and collapse */
	g = lwgeom_from_wkt("POLYGON((0 0, 1 0, 1 1, 0 1, 0 0))", LW_PARSER_CHECK_NONE);
	l = lwgeom_simplify(g, 10, LW_FALSE);
	CU_ASSERT_EQUAL(l, NULL);
	lwgeom_free(g);
	lwgeom_free(l);

	/* Not simplifiable */
	g = lwgeom_from_wkt("LINESTRING(0 0, 50 1.00001, 100 0)", LW_PARSER_CHECK_NONE);
	l = lwgeom_simplify(g, 1.0, LW_FALSE);
	ewkt = lwgeom_to_ewkt(l);
	CU_ASSERT_STRING_EQUAL(ewkt, "LINESTRING(0 0,50 1.00001,100 0)");
	lwgeom_free(g);
	lwgeom_free(l);
	lwfree(ewkt);

	/* Simplifiable */
	g = lwgeom_from_wkt("LINESTRING(0 0,50 0.99999,100 0)", LW_PARSER_CHECK_NONE);
	l = lwgeom_simplify(g, 1.0, LW_FALSE);
	ewkt = lwgeom_to_ewkt(l);
	CU_ASSERT_STRING_EQUAL(ewkt, "LINESTRING(0 0,100 0)");
	lwgeom_free(g);
	lwgeom_free(l);
	lwfree(ewkt);
}


static void do_median_dims_check(char* wkt, int expected_dims)
{
	LWGEOM* g = lwgeom_from_wkt(wkt, LW_PARSER_CHECK_NONE);
	LWPOINT* result = lwgeom_median(g, 1e-8, 100, LW_FALSE);

	CU_ASSERT_EQUAL(expected_dims, lwgeom_ndims((LWGEOM*) result));

	lwgeom_free(g);
	lwpoint_free(result);
}

static void test_median_handles_3d_correctly(void)
{
	do_median_dims_check("MULTIPOINT ((1 3), (4 7), (2 9), (0 4), (2 2))", 2);
	do_median_dims_check("MULTIPOINT Z ((1 3 4), (4 7 8), (2 9 1), (0 4 4), (2 2 3))", 3);
	do_median_dims_check("MULTIPOINT M ((1 3 4), (4 7 8), (2 9 1), (0 4 4), (2 2 3))", 2);
	do_median_dims_check("MULTIPOINT ZM ((1 3 4 5), (4 7 8 6), (2 9 1 7), (0 4 4 8), (2 2 3 9))", 3);
}

static void do_median_test(char* input, char* expected, int fail_if_not_converged, int iter_count)
{
	cu_error_msg_reset();
	LWGEOM* g = lwgeom_from_wkt(input, LW_PARSER_CHECK_NONE);
	LWPOINT* expected_result = NULL;
	POINT4D actual_pt;
	POINT4D expected_pt;

	LWPOINT* result = lwgeom_median(g, FP_TOLERANCE / 10.0, iter_count, fail_if_not_converged);
	int passed = LW_FALSE;

	if (expected != NULL)
	{
		expected_result = lwgeom_as_lwpoint(lwgeom_from_wkt(expected, LW_PARSER_CHECK_NONE));
		lwpoint_getPoint4d_p(expected_result, &expected_pt);
	}
	if (result != NULL)
	{
		lwpoint_getPoint4d_p(result, &actual_pt);
	}

	if (result != NULL && expected != NULL) /* got something, expecting something */
	{
		passed = LW_TRUE;
		passed = passed && lwgeom_is_empty((LWGEOM*) expected_result) == lwgeom_is_empty((LWGEOM*) result);
		passed = passed && (lwgeom_has_z((LWGEOM*) expected_result) == lwgeom_has_z((LWGEOM*) result));
		if (!lwgeom_is_empty((LWGEOM*) result))
		{
			passed = passed && FP_EQUALS(actual_pt.x, expected_pt.x);
			passed = passed && FP_EQUALS(actual_pt.y, expected_pt.y);
			passed = passed && (!lwgeom_has_z((LWGEOM*) expected_result) || FP_EQUALS(actual_pt.z, expected_pt.z));
			passed = passed && (!lwgeom_has_m((LWGEOM*) expected_result) || FP_EQUALS(actual_pt.m, expected_pt.m));
		}
		if (!passed)
			printf("median_test input %s (parsed %s) expected %s got %s\n", input, lwgeom_to_ewkt(g), lwgeom_to_ewkt((LWGEOM*) expected_result), lwgeom_to_ewkt((LWGEOM*) result));
	}
	else if (result == NULL && expected == NULL) /* got nothing, expecting nothing */
	{
		passed = LW_TRUE;
	}
	else if (result != NULL && expected == NULL) /* got something, expecting nothing */
	{
		passed = LW_FALSE;
		printf("median_test input %s (parsed %s) expected NULL got %s\n", input, lwgeom_to_ewkt(g), lwgeom_to_ewkt((LWGEOM*) result));
	}
	else if (result == NULL && expected != NULL) /* got nothing, expecting something */
	{
		passed = LW_FALSE;
		printf("%s", cu_error_msg);
		printf("median_test input %s (parsed %s) expected %s got NULL\n", input, lwgeom_to_ewkt(g), lwgeom_to_ewkt((LWGEOM*) expected_result));
	}

	CU_ASSERT_TRUE(passed);

	lwgeom_free(g);
	lwpoint_free(expected_result);
	lwpoint_free(result);
}

static void test_median_robustness(void)
{
	/* A simple implementation of Weiszfeld's algorithm will fail if the median is equal
	 * to any one of the inputs, during any iteration of the algorithm.
	 *
	 * Because the algorithm uses the centroid as a starting point, this situation will
	 * occur in the test case below.
	 */
	do_median_test("MULTIPOINT ((0 -1), (0 0), (0 1))", "POINT (0 0)", LW_TRUE, 1000);

	/* Same as above but 3D, and shifter */
	do_median_test("MULTIPOINT ((1 -1 3), (1 0 2), (2 1 1))", "POINT (1 0 2)", LW_TRUE, 1000);

	/* Starting point is duplicated */
	do_median_test("MULTIPOINT ((0 -1), (0 0), (0 0), (0 1))", "POINT (0 0)", LW_TRUE, 1000);

	/* Cube */
	do_median_test("MULTIPOINT ((10 10 10), (10 20 10), (20 10 10), (20 20 10), (10 10 20), (10 20 20), (20 10 20), (20 20 20))",
				   "POINT (15 15 15)", LW_TRUE, 1000);

	/* Some edge cases */
	do_median_test("POINT (7 6)", "POINT (7 6)", LW_TRUE, 1000);
	do_median_test("POINT (7 6 2)", "POINT (7 6 2)", LW_TRUE, 1000);
	do_median_test("MULTIPOINT ((7 6 2), EMPTY)", "POINT (7 6 2)", LW_TRUE, 1000);

	/* Empty input */
	do_median_test("MULTIPOINT EMPTY", "POINT EMPTY", LW_FALSE, 1000);
	do_median_test("MULTIPOINT (EMPTY)", "POINT EMPTY", LW_FALSE, 1000);
	do_median_test("MULTIPOINT EMPTY", NULL, LW_TRUE, 1000);
	do_median_test("MULTIPOINT (EMPTY)", NULL, LW_TRUE, 1000);

	/* Weighted input */
	do_median_test("MULTIPOINT ZM (1 -1 3 1, 1 0 2 7, 2 1 1 1)", "POINT (1 0 2)", LW_TRUE, 1000);
	do_median_test("MULTIPOINT ZM (-1 1 -3 1, -1 0 -2 7, -2 -1 -1 1)", "POINT (-1 0 -2)", LW_TRUE, 1000);
	do_median_test("MULTIPOINT ZM (-1 1 -3 1, -1 0 -2 7, -2 -1 -1 0.5, -2 -1 -1 0.5)", "POINT (-1 0 -2)", LW_TRUE, 1000);
	
	/* Point that is replaced by two half-weighted */
	do_median_test("MULTIPOINT ZM ((0 -1 0 1), (0 0 0 1), (0 1 0 0.5), (0 1 0 0.5))", "POINT (0 0 0)", LW_TRUE, 1000);
	/* Point is doubled and then erased by negative weight */
	do_median_test("MULTIPOINT ZM ((1 -1 3 1), (1 0 2 7), (2 1 1 2), (2 1 1 -1))", NULL, LW_TRUE, 1000);
	do_median_test("MULTIPOINT ZM ((1 -1 3 1), (1 0 2 7), (2 1 1 2), (2 1 1 -1))", NULL, LW_FALSE, 1000);
	/* Weightless input is empty */
	do_median_test("MULTIPOINT ZM ((0 -1 0 0), (0 0 0 0), (0 0 0 0), (0 1 0 0))", "POINT EMPTY", LW_FALSE, 1000);
	do_median_test("MULTIPOINT ZM ((0 -1 0 0), (0 0 0 0), (0 0 0 0), (0 1 0 0))", NULL, LW_TRUE, 1000);
	/* Negative weight won't converge */
	do_median_test("MULTIPOINT ZM ((0 -1 0 -1), (0 0 0 -1), (0 1 0 -1))", NULL, LW_FALSE, 1000);
	do_median_test("MULTIPOINT ZM ((0 -1 0 -1), (0 0 0 -1), (0 1 0 -1))", NULL, LW_TRUE, 1000);

	/* Bind convergence too tightly */
	do_median_test("MULTIPOINT ((0 0), (1 1), (0 1), (2 2))", "POINT(0.75 1.0)", LW_FALSE, 0);
	do_median_test("MULTIPOINT ((0 0), (1 1), (0 1), (2 2))", NULL, LW_TRUE, 1);
	/* Unsupported geometry type */
	do_median_test("POLYGON((1 0,0 1,1 2,2 1,1 0))", NULL, LW_TRUE, 1000);
	do_median_test("POLYGON((1 0,0 1,1 2,2 1,1 0))", NULL, LW_FALSE, 1000);
}

static void test_point_density(void)
{
	LWGEOM *geom;
	LWMPOINT *mpt;
	// char *ewkt;

	/* POLYGON */
	geom = lwgeom_from_wkt("POLYGON((1 0,0 1,1 2,2 1,1 0))", LW_PARSER_CHECK_NONE);
	mpt = lwgeom_to_points(geom, 100);
	CU_ASSERT_EQUAL(mpt->ngeoms,100);
	// ewkt = lwgeom_to_ewkt((LWGEOM*)mpt);
	// printf("%s\n", ewkt);
	// lwfree(ewkt);
	lwmpoint_free(mpt);

	mpt = lwgeom_to_points(geom, 1);
	CU_ASSERT_EQUAL(mpt->ngeoms,1);
	lwmpoint_free(mpt);

	mpt = lwgeom_to_points(geom, 0);
	CU_ASSERT_EQUAL(mpt, NULL);
	lwmpoint_free(mpt);

	lwgeom_free(geom);

	/* MULTIPOLYGON */
	geom = lwgeom_from_wkt("MULTIPOLYGON(((10 0,0 10,10 20,20 10,10 0)),((0 0,5 0,5 5,0 5,0 0)))", LW_PARSER_CHECK_NONE);

	mpt = lwgeom_to_points(geom, 1000);
	CU_ASSERT_EQUAL(mpt->ngeoms,1000);
	lwmpoint_free(mpt);

	mpt = lwgeom_to_points(geom, 1);
	CU_ASSERT_EQUAL(mpt->ngeoms,1);
	lwmpoint_free(mpt);

	lwgeom_free(geom);
}

static void test_lwpoly_construct_circle(void)
{
	LWPOLY* p;
	const GBOX* g;
	const int srid = 4326;
	const int segments_per_quad = 17;
	const int x = 10;
	const int y = 20;
	const int r = 5;

	/* With normal arguments you should get something circle-y */
	p = lwpoly_construct_circle(srid, x, y, r, segments_per_quad, LW_TRUE);

	ASSERT_INT_EQUAL(lwgeom_count_vertices(lwpoly_as_lwgeom(p)), segments_per_quad * 4 + 1);
	ASSERT_INT_EQUAL(lwgeom_get_srid(lwpoly_as_lwgeom(p)), srid)

	g = lwgeom_get_bbox(lwpoly_as_lwgeom(p));
	CU_ASSERT_DOUBLE_EQUAL(g->xmin, x-r, 0.1);
	CU_ASSERT_DOUBLE_EQUAL(g->xmax, x+r, 0.1);
	CU_ASSERT_DOUBLE_EQUAL(g->ymin, y-r, 0.1);
	CU_ASSERT_DOUBLE_EQUAL(g->ymax, y+r, 0.1);

	CU_ASSERT_DOUBLE_EQUAL(lwgeom_area(lwpoly_as_lwgeom(p)), M_PI*5*5, 0.1);

	lwpoly_free(p);

	/* No segments? No circle. */
	p = lwpoly_construct_circle(srid, x, y, r, 0, LW_TRUE);
	CU_ASSERT_TRUE(p == NULL);

	/* Negative radius? No circle. */
	p = lwpoly_construct_circle(srid, x, y, -1e-3, segments_per_quad, LW_TRUE);
	CU_ASSERT_TRUE(p == NULL);

	/* Zero radius? Invalid circle */
	p = lwpoly_construct_circle(srid, x, y, 0, segments_per_quad, LW_TRUE);
	CU_ASSERT_TRUE(p != NULL);
	lwpoly_free(p);
}

static void test_kmeans(void)
{
	static int cluster_size = 100;
	static int num_clusters = 4;
	static double spread = 1.5;
	int N = cluster_size * num_clusters;
	LWGEOM **geoms;
	int i, j, k=0;
	int *r;

	geoms = lwalloc(sizeof(LWGEOM*) * N);

	for (j = 0; j < num_clusters; j++) {
		for (i = 0; i < cluster_size; i++)
		{
			double u1 = 1.0 * rand() / RAND_MAX;
			double u2 = 1.0 * rand() / RAND_MAX;
			double z1 = spread * j + sqrt(-2*log2(u1))*cos(2*M_PI*u2);
			double z2 = spread * j + sqrt(-2*log2(u1))*sin(2*M_PI*u2);

			LWPOINT *lwp = lwpoint_make2d(SRID_UNKNOWN, z1, z2);
			geoms[k++] = lwpoint_as_lwgeom(lwp);
		}
	}

	r = lwgeom_cluster_2d_kmeans((const LWGEOM **)geoms, N, num_clusters);

	// for (i = 0; i < k; i++)
	// {
	// 	printf("[%d] %s\n", r[i], lwgeom_to_ewkt(geoms[i]));
	// }

	/* Clean up */
	lwfree(r);
	for (i = 0; i < k; i++)
		lwgeom_free(geoms[i]);
	lwfree(geoms);

	return;
}

/*
** Used by test harness to register the tests in this file.
*/
void algorithms_suite_setup(void);
void algorithms_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("algorithm", init_cg_suite, clean_cg_suite);
	PG_ADD_TEST(suite,test_lw_segment_side);
	PG_ADD_TEST(suite,test_lw_segment_intersects);
	PG_ADD_TEST(suite,test_lwline_crossing_short_lines);
	PG_ADD_TEST(suite,test_lwline_crossing_long_lines);
	PG_ADD_TEST(suite,test_lwline_crossing_bugs);
	PG_ADD_TEST(suite,test_lwpoint_set_ordinate);
	PG_ADD_TEST(suite,test_lwpoint_get_ordinate);
	PG_ADD_TEST(suite,test_point_interpolate);
	PG_ADD_TEST(suite,test_lwline_interpolate_points);
	PG_ADD_TEST(suite,test_lwline_clip);
	PG_ADD_TEST(suite,test_lwline_clip_big);
	PG_ADD_TEST(suite,test_lwmline_clip);
	PG_ADD_TEST(suite,test_geohash_point);
	PG_ADD_TEST(suite,test_geohash_precision);
	PG_ADD_TEST(suite,test_geohash);
	PG_ADD_TEST(suite,test_geohash_point_as_int);
	PG_ADD_TEST(suite,test_isclosed);
	PG_ADD_TEST(suite,test_lwgeom_simplify);
	PG_ADD_TEST(suite,test_lw_arc_center);
	PG_ADD_TEST(suite,test_point_density);
	PG_ADD_TEST(suite,test_kmeans);
	PG_ADD_TEST(suite,test_median_handles_3d_correctly);
	PG_ADD_TEST(suite,test_median_robustness);
	PG_ADD_TEST(suite,test_lwpoly_construct_circle);
	PG_ADD_TEST(suite,test_lwgeom_remove_repeated_points);
}
