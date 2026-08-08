BEGIN;

DROP EXTENSION postgis_topology;

CREATE TABLE public.topology_install_overload_marker (
    invoked boolean NOT NULL
);

GRANT INSERT ON public.topology_install_overload_marker TO PUBLIC;
GRANT USAGE, CREATE ON SCHEMA topology TO PUBLIC;

CREATE FUNCTION topology.addtosearchpath(text)
RETURNS text
LANGUAGE plpgsql
AS $marker$
BEGIN
    INSERT INTO public.topology_install_overload_marker VALUES (true);
    RETURN $1;
END
$marker$;

CREATE EXTENSION postgis_topology;

COMMIT;
