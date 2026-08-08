SELECT 'install_addtosearchpath',
    CASE WHEN
        pg_catalog.to_regprocedure('topology.addtosearchpath(text)') IS NOT NULL
        AND EXISTS (
            SELECT 1
            FROM pg_catalog.pg_extension
            WHERE extname = 'postgis_topology'
        )
        AND NOT EXISTS (
            SELECT 1
            FROM public.topology_install_overload_marker
        )
        THEN 'safe'
        ELSE 'unsafe'
    END;
