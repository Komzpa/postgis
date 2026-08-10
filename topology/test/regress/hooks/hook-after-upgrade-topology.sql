SELECT * FROM topology.layer;
\d upgrade_test.feature

DO $edge_trigger_upgrade$
DECLARE
	inserted_edge_id bigint;
	base_trigger_count integer;
	view_trigger_count integer;
BEGIN
	IF EXISTS (
		SELECT 1
		FROM pg_catalog.pg_rewrite
		WHERE ev_class = 'upgrade_edge_triggers.edge'::regclass
		  AND rulename = 'edge_insert_rule'
	) THEN
		RAISE EXCEPTION 'Topology upgrade left edge_insert_rule installed';
	END IF;

	SELECT pg_catalog.count(*)
	INTO base_trigger_count
	FROM pg_catalog.pg_trigger
	WHERE tgrelid = 'upgrade_edge_triggers.edge_data'::regclass
	  AND tgname = 'edge_data_abs_next_edges'
	  AND NOT tgisinternal;

	SELECT pg_catalog.count(*)
	INTO view_trigger_count
	FROM pg_catalog.pg_trigger
	WHERE tgrelid = 'upgrade_edge_triggers.edge'::regclass
	  AND tgname = 'edge_insert'
	  AND NOT tgisinternal;

	IF base_trigger_count <> 1 OR view_trigger_count <> 1 THEN
		RAISE EXCEPTION
			'Topology upgrade installed base/view trigger counts %/%',
			base_trigger_count, view_trigger_count;
	END IF;

	IF EXISTS (
		SELECT 1
		FROM pg_catalog.pg_depend dep
		JOIN pg_catalog.pg_extension ext ON ext.oid = dep.refobjid
		WHERE dep.deptype = 'e'
		  AND ext.extname = 'postgis_topology'
		  AND (
			(dep.classid = 'pg_catalog.pg_trigger'::regclass AND dep.objid IN (
				SELECT oid
				FROM pg_catalog.pg_trigger
				WHERE tgrelid IN (
					'upgrade_edge_triggers.edge'::regclass,
					'upgrade_edge_triggers.edge_data'::regclass
				)
				  AND NOT tgisinternal
			))
			OR
			(dep.classid = 'pg_catalog.pg_proc'::regclass AND dep.objid =
				'upgrade_edge_triggers._edge_insert()'::regprocedure)
		  )
	) THEN
		RAISE EXCEPTION 'Per-topology edge trigger objects remain extension members';
	END IF;

	INSERT INTO upgrade_edge_triggers.edge(
		edge_id, start_node, end_node,
		next_left_edge, next_right_edge,
		left_face, right_face, geom
	)
	VALUES (
		100, 100, 101, -100, 100, 0, 0,
		'LINESTRING(0 0, 1 0)'::geometry
	)
	RETURNING edge_id INTO inserted_edge_id;

	IF inserted_edge_id <> 100 OR NOT EXISTS (
		SELECT 1
		FROM upgrade_edge_triggers.edge_data
		WHERE edge_id = 100
		  AND abs_next_left_edge = 100
		  AND abs_next_right_edge = 100
	) THEN
		RAISE EXCEPTION 'Upgraded edge view insert did not maintain absolute links';
	END IF;

	INSERT INTO upgrade_edge_triggers.edge_data(
		edge_id, start_node, end_node,
		next_left_edge, abs_next_left_edge,
		next_right_edge, abs_next_right_edge,
		left_face, right_face, geom
	)
	VALUES (
		101, 101, 100,
		101, 0, -101, 0,
		0, 0, 'LINESTRING(1 0, 0 0)'::geometry
	);

	UPDATE upgrade_edge_triggers.edge_data
	SET abs_next_left_edge = 0,
		abs_next_right_edge = 0
	WHERE edge_id = 101;

	IF NOT EXISTS (
		SELECT 1
		FROM upgrade_edge_triggers.edge_data
		WHERE edge_id = 101
		  AND abs_next_left_edge = 101
		  AND abs_next_right_edge = 101
	) THEN
		RAISE EXCEPTION 'Upgraded edge_data trigger did not maintain absolute links';
	END IF;
END
$edge_trigger_upgrade$ LANGUAGE plpgsql;

SELECT topology.DropTopology('upgrade_edge_triggers');

DO $$
DECLARE
	call_count integer;
BEGIN
	SELECT calls.call_count
	INTO call_count
	FROM public.upgrade_test_pg_get_function_result_calls AS calls;

	IF call_count <> 0 THEN
		RAISE EXCEPTION
			'public.pg_get_function_result(regprocedure) was called % time(s) during extension upgrade',
			call_count;
	END IF;
END;
$$ LANGUAGE plpgsql;

DROP FUNCTION public.pg_get_function_result(regprocedure);
DROP TABLE public.upgrade_test_pg_get_function_result_calls;

DO $test$
BEGIN
	IF EXISTS (
		SELECT 1 FROM public.upgrade_add_column_overload_marker
	) THEN
		RAISE EXCEPTION
			'postgis_topology update called the public text overload';
	END IF;

	IF NOT EXISTS (
		SELECT 1
		FROM information_schema.columns
		WHERE table_schema = 'topology'
		  AND table_name = 'topology'
		  AND column_name = 'useslargeids'
		  AND data_type = 'boolean'
		  AND is_nullable = 'NO'
		  AND column_default = 'false'
	) THEN
		RAISE EXCEPTION
			'postgis_topology update did not add useslargeids safely';
	END IF;
END
$test$;

DROP FUNCTION public._postgis_add_column_to_table(
	text, text, text, boolean, text, text
);
DROP TABLE public.upgrade_add_column_overload_marker;

DO $postgis_upgrade_test$
BEGIN
	IF EXISTS (
		SELECT 1
		FROM upgrade_test.domain_constraint_callback
	) THEN
		RAISE EXCEPTION 'Topology upgrade invoked a non-catalog domain constraint callback';
	END IF;
END
$postgis_upgrade_test$;

DROP FUNCTION public.array_upper(bigint[], integer);
DROP FUNCTION public.array_lower(bigint[], integer);

-- https://trac.osgeo.org/postgis/ticket/5983
DROP INDEX upgrade_test.upgrade_test_feature_tg_id_idx;
SELECT topology.FixCorruptTopoGeometryColumn(schema_name, table_name, feature_column)
    FROM topology.layer;

\d upgrade_test.feature

-- See https://trac.osgeo.org/postgis/ticket/5102
SELECT topology.CopyTopology('upgrade_test', 'upgrade_test_copy');
INSERT INTO upgrade_test.domain_test values (
  '{1,2}'::topology.topoelement,
  '{{2,3}}'::topology.topoelementarray
);

SELECT * FROM topology.layer;

INSERT INTO upgrade_test.domain_test values (
  '{1,2}'::topology.topoelement,
  '{{2,3}}'::topology.topoelementarray
);

SELECT topology.DropTopology('upgrade_test');
SELECT topology.DropTopology('upgrade_test_copy');
