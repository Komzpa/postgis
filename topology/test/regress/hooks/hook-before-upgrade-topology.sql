SELECT topology.createTopology('upgrade_test');

-- Simulate a topology created before #5119 and #5334. The extension upgrade
-- must replace the edge view rule and install both trigger layers.
SELECT topology.createTopology('upgrade_edge_triggers');
DROP TRIGGER edge_insert ON upgrade_edge_triggers.edge;
DROP FUNCTION upgrade_edge_triggers._edge_insert();
DROP TRIGGER edge_data_abs_next_edges ON upgrade_edge_triggers.edge_data;
CREATE RULE edge_insert_rule AS
ON INSERT TO upgrade_edge_triggers.edge
DO INSTEAD INSERT INTO upgrade_edge_triggers.edge_data
VALUES (
	NEW.edge_id, NEW.start_node, NEW.end_node,
	NEW.next_left_edge, pg_catalog.abs(NEW.next_left_edge),
	NEW.next_right_edge, pg_catalog.abs(NEW.next_right_edge),
	NEW.left_face, NEW.right_face, NEW.geom
);
INSERT INTO upgrade_edge_triggers.node(node_id, containing_face, geom)
VALUES
	(100, NULL, 'POINT(0 0)'::geometry),
	(101, NULL, 'POINT(1 0)'::geometry);

-- Create some TopoGeometry data
CREATE TABLE upgrade_test.feature(id serial primary key);
SELECT topology.AddTopoGeometryColumn('upgrade_test', 'upgrade_test', 'feature', 'tg', 'linear');
INSERT INTO upgrade_test.feature(tg) SELECT topology.toTopoGeom('LINESTRING(0 0, 10 0)', 'upgrade_test', 1);
CREATE INDEX upgrade_test_feature_tg_id_idx ON upgrade_test.feature ( id(tg) );

-- Create some TopoGeometry data
CREATE TABLE upgrade_test.domain_test(a topology.topoelement, b topology.topoelementarray);
INSERT INTO upgrade_test.domain_test values (
  '{1,2}'::topology.topoelement,
  '{{2,3}}'::topology.topoelementarray
);

-- An unqualified pg_get_function_result(regprocedure) call in an upgrade
-- script can select this exact-match overload instead of the pg_catalog
-- function, whose argument is oid. Keep the overload harmless: record the
-- call and delegate to the intended function.
CREATE TABLE public.upgrade_test_pg_get_function_result_calls (
	call_count integer NOT NULL
);
INSERT INTO public.upgrade_test_pg_get_function_result_calls VALUES (0);

CREATE FUNCTION public.pg_get_function_result(function_oid regprocedure)
RETURNS text
LANGUAGE plpgsql
AS $$
BEGIN
	UPDATE public.upgrade_test_pg_get_function_result_calls
	SET call_count = call_count + 1;
	RETURN pg_catalog.pg_get_function_result(function_oid::oid);
END;
$$;

-- Simulate the pre-3.6.0 catalog state that needs the upgrade helper.
ALTER TABLE topology.topology DROP COLUMN IF EXISTS useslargeids;

-- This harmless overload records an unsafe function-resolution result.
CREATE TABLE public.upgrade_add_column_overload_marker (
	called boolean NOT NULL
);

CREATE FUNCTION public._postgis_add_column_to_table(
	table_name text,
	column_name text,
	data_type text,
	is_not_null boolean,
	default_value text,
	deprecated_in_version text
)
RETURNS void
LANGUAGE plpgsql
AS $marker$
BEGIN
	INSERT INTO public.upgrade_add_column_overload_marker VALUES (true);
END
$marker$;

-- Keep the callback harmless. The after-upgrade hook rejects any marker row.
CREATE TABLE upgrade_test.domain_constraint_callback (
	callback_name text NOT NULL
);

CREATE FUNCTION public.array_upper(bigint[], integer)
RETURNS integer
LANGUAGE plpgsql
SECURITY INVOKER
AS $callback$
BEGIN
	INSERT INTO upgrade_test.domain_constraint_callback VALUES ('array_upper');
	RETURN pg_catalog.array_upper($1, $2);
END
$callback$;

CREATE FUNCTION public.array_lower(bigint[], integer)
RETURNS integer
LANGUAGE plpgsql
SECURITY INVOKER
AS $callback$
BEGIN
	INSERT INTO upgrade_test.domain_constraint_callback VALUES ('array_lower');
	RETURN pg_catalog.array_lower($1, $2);
END
$callback$;
