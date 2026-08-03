-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--
--
-- PostGIS - Spatial Types for PostgreSQL
-- http://postgis.net
--
-- Copyright (C) 2011-2020 Sandro Santilli <strk@kbt.io>
-- Copyright (C) 2010-2012 Regina Obe <lr@pcorp.us>
-- Copyright (C) 2009      Paul Ramsey <pramsey@cleverelephant.ca>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
--
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--
-- This file will be appended at the very end of every
-- sql upgrade script.
--
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DO LANGUAGE 'plpgsql'
$POSTGIS_TOPOLOGY_NESTED_DOMAIN_DEFAULT_RESTORE$
DECLARE
	rec RECORD;
	sql TEXT;
BEGIN
	FOR rec IN
		WITH RECURSIVE topology_domain(type_oid) AS (
			SELECT t.oid
			FROM pg_catalog.pg_type AS t
			JOIN pg_catalog.pg_namespace AS n
				ON n.oid = t.typnamespace
			WHERE n.nspname = 'topology'
			AND t.typname IN ('topoelement', 'topoelementarray')
			UNION
			SELECT t.oid
			FROM topology_domain
			JOIN pg_catalog.pg_type AS t
				ON t.typbasetype = topology_domain.type_oid
			WHERE t.typtype = 'd'
		)
		SELECT
			n.nspname AS domain_schema,
			t.typname AS domain_name,
			t.oid::regtype::text AS domain_type,
			pg_catalog.pg_get_expr(t.typdefaultbin, 0) AS default_expr
		FROM topology_domain
		JOIN pg_catalog.pg_type AS t
			ON t.oid = topology_domain.type_oid
		JOIN pg_catalog.pg_namespace AS n
			ON n.oid = t.typnamespace
		WHERE t.typtype = 'd'
		AND t.typdefaultbin IS NOT NULL
		AND pg_catalog.pg_get_expr(t.typdefaultbin, 0) NOT LIKE '%::text)%'
	LOOP
		sql := pg_catalog.format(
			'ALTER DOMAIN %I.%I DROP DEFAULT',
			rec.domain_schema,
			rec.domain_name
		);
		EXECUTE sql;

		sql := pg_catalog.format(
			'ALTER DOMAIN %I.%I SET DEFAULT ((%s)::text::%s)',
			rec.domain_schema,
			rec.domain_name,
			rec.default_expr,
			rec.domain_type
		);
		EXECUTE sql;
	END LOOP;
END
$POSTGIS_TOPOLOGY_NESTED_DOMAIN_DEFAULT_RESTORE$;

DO LANGUAGE 'plpgsql'
$POSTGIS_TOPOLOGY_COLUMN_DEFAULT_RESTORE$
DECLARE
	rec RECORD;
	sql TEXT;
BEGIN
	FOR rec IN
		WITH RECURSIVE topology_domain(type_oid) AS (
			SELECT t.oid
			FROM pg_catalog.pg_type AS t
			JOIN pg_catalog.pg_namespace AS n
				ON n.oid = t.typnamespace
			WHERE n.nspname = 'topology'
			AND t.typname IN ('topoelement', 'topoelementarray')
			UNION
			SELECT t.oid
			FROM topology_domain
			JOIN pg_catalog.pg_type AS t
				ON t.typbasetype = topology_domain.type_oid
			WHERE t.typtype = 'd'
		)
		SELECT
			a.attrelid,
			a.attname,
			CASE
				WHEN at.typelem = ANY(pg_catalog.array_agg(topology_domain.type_oid) OVER ())
				THEN a.atttypid::regtype::text
				ELSE at.typbasetype::regtype::text
			END AS target_type,
			pg_catalog.pg_get_expr(d.adbin, d.adrelid) AS default_expr,
			CASE
				WHEN at.typelem = ANY(pg_catalog.array_agg(topology_domain.type_oid) OVER ())
				THEN 'pg_catalog.text[]'
				ELSE 'pg_catalog.text'
			END AS text_type
		FROM pg_catalog.pg_attribute AS a
		JOIN pg_catalog.pg_attrdef AS d
			ON d.adrelid = a.attrelid
			AND d.adnum = a.attnum
		JOIN pg_catalog.pg_type AS at
			ON at.oid = a.atttypid
		JOIN topology_domain
			ON a.atttypid = topology_domain.type_oid
			OR at.typelem = topology_domain.type_oid
		WHERE a.attnum > 0
		AND NOT a.attisdropped
		AND a.attgenerated = ''
	LOOP
		IF rec.text_type = 'pg_catalog.text[]'
			AND rec.default_expr LIKE '%::text[])::%'
		THEN
			CONTINUE;
		END IF;

		IF rec.text_type = 'pg_catalog.text'
			AND rec.default_expr LIKE '%::text)::%'
			AND rec.default_expr LIKE '%' || rec.target_type || '%'
		THEN
			CONTINUE;
		END IF;

		sql := pg_catalog.format(
			'ALTER TABLE %s ALTER COLUMN %I SET DEFAULT ((%s)::%s::%s)',
			rec.attrelid::regclass,
			rec.attname,
			rec.default_expr,
			rec.text_type,
			rec.target_type
		);
		EXECUTE sql;
	END LOOP;
END
$POSTGIS_TOPOLOGY_COLUMN_DEFAULT_RESTORE$;

DO LANGUAGE 'plpgsql'
$POSTGIS_TOPOLOGY_DOMAIN_CONSTRAINT_RESTORE$
DECLARE
	rec RECORD;
	domain_constraint RECORD;
	sql TEXT;
	constraint_not_valid_marker TEXT := 'postgis-topology-domain-constraint-not-valid-by-repair-306';
BEGIN
	FOR rec IN
		WITH RECURSIVE topology_domain(type_oid) AS (
			SELECT t.oid
			FROM pg_catalog.pg_type AS t
			JOIN pg_catalog.pg_namespace AS n
				ON n.oid = t.typnamespace
			WHERE n.nspname = 'topology'
			AND t.typname IN ('topoelement', 'topoelementarray')
			UNION
			SELECT t.oid
			FROM topology_domain
			JOIN pg_catalog.pg_type AS t
				ON t.typbasetype = topology_domain.type_oid
			WHERE t.typtype = 'd'
		)
		SELECT
			t.oid AS domain_oid,
			n.nspname AS domain_schema,
			t.typname AS domain_name
		FROM topology_domain
		JOIN pg_catalog.pg_type AS t
			ON t.oid = topology_domain.type_oid
		JOIN pg_catalog.pg_namespace AS n
			ON n.oid = t.typnamespace
		AND EXISTS (
			SELECT 1
			FROM pg_catalog.pg_attribute AS a
			JOIN pg_catalog.pg_class AS c
				ON c.oid = a.attrelid
			JOIN pg_catalog.pg_type AS at
				ON at.oid = a.atttypid
			WHERE (
				a.atttypid = t.typarray
				OR at.typelem = t.oid
				OR a.atttypid = t.oid
			)
			AND a.attnum > 0
			AND NOT a.attisdropped
			AND c.relkind IN ('r', 'p', 'm', 'c')
		)
	LOOP
		FOR domain_constraint IN
			SELECT
				con.conname,
				pg_catalog.regexp_replace(
					pg_catalog.pg_get_constraintdef(con.oid),
					'[[:space:]]+NOT[[:space:]]+VALID[[:space:]]*$',
					'',
					'i'
				) AS constraint_def
			FROM pg_catalog.pg_constraint AS con
			WHERE con.contypid = rec.domain_oid
			AND con.convalidated
			-- Only demote when the storage really is still 32 bit, and read that
			-- from the catalog rather than from anybody's comment: the domain's
			-- base type is `integer[]` before the repair and `bigint[]` after it,
			-- so this asks the one question that matters and cannot be forged.
			--
			-- Two earlier attempts at this guard were wrong, both caught by an
			-- adversarial review of this very block. Keying on a *marked*
			-- constraint accepted a comment a user had written by hand as proof
			-- that a repair ran, and then demoted every validated constraint on
			-- the domain and overwrote their comments. Keying on the absence of
			-- the domain's repair marker punished databases that never needed a
			-- repair, including fresh installs. And a constraint left NOT VALID
			-- is not evidence either: PostgreSQL refuses to validate a
			-- root-domain constraint while a direct `topoelement[]` carrier
			-- exists, so a *complete* repair leaves that mark for a structural
			-- reason and the next upgrade must not read it as old-width storage.
			--
			-- Without any guard at all the loop can only do damage: it selects
			-- `con.convalidated`, which is exactly the set the repair chose to
			-- leave validated, and re-adds every one of them NOT VALID. Nothing
			-- puts the validity back -- there is no VALIDATE CONSTRAINT anywhere
			-- in the tree -- and on a domain with an array-of-domain column
			-- PostgreSQL refuses to validate at all, so the demotion is
			-- permanent.
			AND EXISTS (
				SELECT 1
				FROM pg_catalog.pg_type AS dom
				WHERE dom.oid = rec.domain_oid
				AND dom.typbasetype = 'pg_catalog.int4[]'::regtype::oid
			)
		LOOP
			sql := pg_catalog.format(
				'ALTER DOMAIN %I.%I DROP CONSTRAINT %I',
				rec.domain_schema,
				rec.domain_name,
				domain_constraint.conname
			);
			EXECUTE sql;

			sql := pg_catalog.format(
				'ALTER DOMAIN %I.%I ADD CONSTRAINT %I %s NOT VALID',
				rec.domain_schema,
				rec.domain_name,
				domain_constraint.conname,
				domain_constraint.constraint_def
			);
			EXECUTE sql;

			sql := pg_catalog.format(
				'COMMENT ON CONSTRAINT %I ON DOMAIN %I.%I IS %L',
				domain_constraint.conname,
				rec.domain_schema,
				rec.domain_name,
				constraint_not_valid_marker
			);
			EXECUTE sql;
		END LOOP;

		FOR domain_constraint IN
			SELECT *
			FROM (
				VALUES
					(
						'topoelement',
						'dimensions',
						'CHECK (array_upper(VALUE, 2) IS NULL AND array_upper(VALUE, 1) = 2)'
					),
					(
						'topoelement',
						'type_range',
						'CHECK (VALUE[2] > 0)'
					),
					(
						'topoelement',
						'lower_dimension',
						'CHECK (array_lower(VALUE, 1) = 1)'
					),
					(
						'topoelementarray',
						'type_range',
						'CHECK (array_upper(VALUE, 2) = 2 AND array_upper(VALUE, 3) IS NULL)'
					)
			) AS canonical_constraint(domain_name, conname, constraint_def)
			WHERE rec.domain_schema = 'topology'
			AND canonical_constraint.domain_name = rec.domain_name
			AND NOT EXISTS (
				SELECT 1
				FROM pg_catalog.pg_constraint AS con
				WHERE con.contypid = rec.domain_oid
				AND con.conname = canonical_constraint.conname
			)
		LOOP
			sql := pg_catalog.format(
				'ALTER DOMAIN %I.%I ADD CONSTRAINT %I %s NOT VALID',
				rec.domain_schema,
				rec.domain_name,
				domain_constraint.conname,
				domain_constraint.constraint_def
			);
			EXECUTE sql;

			sql := pg_catalog.format(
				'COMMENT ON CONSTRAINT %I ON DOMAIN %I.%I IS %L',
				domain_constraint.conname,
				rec.domain_schema,
				rec.domain_name,
				constraint_not_valid_marker
			);
			EXECUTE sql;
		END LOOP;
	END LOOP;
END
$POSTGIS_TOPOLOGY_DOMAIN_CONSTRAINT_RESTORE$;

-- DROP auxiliary function (created by common_before_upgrade.sql)
DROP FUNCTION _postgis_drop_function_by_identity(text, text, text);
DROP FUNCTION _postgis_drop_function_by_signature(text, text);
DROP FUNCTION _postgis_topology_upgrade_domain_type(text, text, text, text);
DROP FUNCTION _postgis_drop_cast_by_types(text, text, text);
DROP FUNCTION _postgis_topology_upgrade_user_type_attribute(text, text, text, text, text);
DROP FUNCTION _postgis_add_column_to_table(regclass, name, text, boolean, text, text);


-- Drop deprecated functions if possible
DO LANGUAGE 'plpgsql'
$POSTGIS_PROC_UPGRADE$
DECLARE
    new_name TEXT;
    rec RECORD;
    extrec RECORD;
    sql TEXT;
    detail TEXT;
    hint TEXT;
BEGIN

    -- Try to drop all deprecated functions,
    -- and report failure to do so as a WARNING
    -- for the user to handle.
    --
    FOR rec IN

        SELECT *, oid::regprocedure as proc
        FROM pg_catalog.pg_proc
        WHERE proname ~ 'deprecated_by_postgis'

    LOOP --{

        RAISE DEBUG 'Handling deprecated function %', rec.proc;

        new_name := pg_catalog.regexp_replace(
            rec.proc::text,
            E'_deprecated_by_postgis[^(]*\\(.*',
            ''
        );

        sql := pg_catalog.format('DROP FUNCTION %s', rec.proc);
        --RAISE DEBUG 'SQL: %', sql;
        BEGIN --{
            EXECUTE sql;
        EXCEPTION
        WHEN OTHERS THEN -- }{
            hint = 'Resolve the issue';
            GET STACKED DIAGNOSTICS detail := PG_EXCEPTION_DETAIL;
            IF detail LIKE '%view % depends%' THEN
                hint = pg_catalog.format(
                    'Replace the view changing all occurrences of %s in its definition with %s',
                    rec.proc,
                    new_name
                );
            END IF;
            hint = hint || ' and upgrade again';

            RAISE WARNING 'Deprecated function % left behind: %',
                rec.proc, SQLERRM
            USING DETAIL = detail, HINT = hint;

            -- Drop the function from any extension it is part of
            -- so dump/reloads still work
            FOR extrec IN
                SELECT e.extname
                FROM
                    pg_catalog.pg_extension e,
                    pg_catalog.pg_depend d
                WHERE
                    d.refclassid = 'pg_catalog.pg_extension'::pg_catalog.regclass AND
                    d.refobjid = e.oid AND
                    d.classid = 'pg_catalog.pg_proc'::pg_catalog.regclass AND
                    d.objid = rec.proc::oid
            LOOP
                RAISE DEBUG 'Unpackaging % from extension %', rec.proc, extrec.extname;
                sql := pg_catalog.format('ALTER EXTENSION %I DROP FUNCTION %s', extrec.extname, rec.proc);
                EXECUTE sql;
            END LOOP;

        END; --}

    END LOOP; --}
END
$POSTGIS_PROC_UPGRADE$;
