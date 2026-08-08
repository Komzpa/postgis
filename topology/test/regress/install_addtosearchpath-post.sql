BEGIN;

REVOKE CREATE ON SCHEMA topology FROM PUBLIC;
DROP FUNCTION topology.addtosearchpath(text);
DROP TABLE public.topology_install_overload_marker;

COMMIT;
