
/*
  +------------------------------------------------------------------------+
  | Phalcon Framework                                                      |
  +------------------------------------------------------------------------+
  | Copyright (c) 2011-2014 Phalcon Team (http://www.phalconphp.com)       |
  +------------------------------------------------------------------------+
  | This source file is subject to the New BSD License that is bundled     |
  | with this package in the file docs/LICENSE.txt.                        |
  |                                                                        |
  | If you did not receive a copy of the license and are unable to         |
  | obtain it through the world-wide-web, please send an email             |
  | to license@phalconphp.com so we can send you a copy immediately.       |
  +------------------------------------------------------------------------+
  | Authors: Andres Gutierrez <andres@phalconphp.com>                      |
  |          Eduar Carvajal <eduar@phalconphp.com>                         |
  |          Marcio Paiva <mpaivabarbosa@gmail.com>                        |
  +------------------------------------------------------------------------+
*/

#include "db/adapter/pdo/oracle.h"
#include "db/adapter/pdo.h"
#include "db/adapterinterface.h"
#include "db/column.h"
#include "db/rawvalue.h"

#include <ext/pdo/php_pdo_driver.h>

#include "kernel/main.h"
#include "kernel/memory.h"
#include "kernel/object.h"
#include "kernel/fcall.h"
#include "kernel/array.h"
#include "kernel/string.h"
#include "kernel/operators.h"
#include "kernel/concat.h"

/**
 * Phalcon\Db\Adapter\Pdo\Oracle
 *
 * Specific functions for the Oracle database system
 * <code>
 *
 * $config = array(
 *  "dbname" => "//localhost/dbname",
 *  "username" => "oracle",
 *  "password" => "oracle"
 * );
 *
 * $connection = new Phalcon\Db\Adapter\Pdo\Oracle($config);
 *
 * </code>
 */
zend_class_entry *phalcon_db_adapter_pdo_oracle_ce;

PHP_METHOD(Phalcon_Db_Adapter_Pdo_Oracle, connect);
PHP_METHOD(Phalcon_Db_Adapter_Pdo_Oracle, describeColumns);
PHP_METHOD(Phalcon_Db_Adapter_Pdo_Oracle, lastInsertId);
PHP_METHOD(Phalcon_Db_Adapter_Pdo_Oracle, useExplicitIdValue);
PHP_METHOD(Phalcon_Db_Adapter_Pdo_Oracle, getDefaultIdValue);
PHP_METHOD(Phalcon_Db_Adapter_Pdo_Oracle, supportSequences);

static const zend_function_entry phalcon_db_adapter_pdo_oracle_method_entry[] = {
	PHP_ME(Phalcon_Db_Adapter_Pdo_Oracle, connect, arginfo_phalcon_db_adapterinterface_connect, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Db_Adapter_Pdo_Oracle, describeColumns, arginfo_phalcon_db_adapterinterface_describecolumns, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Db_Adapter_Pdo_Oracle, lastInsertId, arginfo_phalcon_db_adapterinterface_lastinsertid, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Db_Adapter_Pdo_Oracle, useExplicitIdValue, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Db_Adapter_Pdo_Oracle, getDefaultIdValue, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Db_Adapter_Pdo_Oracle, supportSequences, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END
};


/**
 * Phalcon\Db\Adapter\Pdo\Oracle initializer
 */
PHALCON_INIT_CLASS(Phalcon_Db_Adapter_Pdo_Oracle){

	PHALCON_REGISTER_CLASS_EX(Phalcon\\Db\\Adapter\\Pdo, Oracle, db_adapter_pdo_oracle, phalcon_db_adapter_pdo_ce, phalcon_db_adapter_pdo_oracle_method_entry, 0);

	zend_declare_property_string(phalcon_db_adapter_pdo_oracle_ce, SL("_type"), "oci", ZEND_ACC_PROTECTED);
	zend_declare_property_string(phalcon_db_adapter_pdo_oracle_ce, SL("_dialectType"), "oracle", ZEND_ACC_PROTECTED);

	zend_class_implements(phalcon_db_adapter_pdo_oracle_ce, 1, phalcon_db_adapterinterface_ce);

	return SUCCESS;
}

/**
 * This method is automatically called in Phalcon\Db\Adapter\Pdo constructor.
 * Call it when you need to restore a database connection.
 *
 * @param array $descriptor
 * @return boolean
 */
PHP_METHOD(Phalcon_Db_Adapter_Pdo_Oracle, connect){

	zval *descriptor = NULL, startup, *value = NULL;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 0, 1, &descriptor);

	if (!descriptor || !zend_is_true(descriptor)) {
		descriptor = phalcon_read_property(getThis(), SL("_descriptor"), PH_NOISY);
	}

	/** 
	 * Connect
	 */
	PHALCON_CALL_PARENT(NULL, phalcon_db_adapter_pdo_oracle_ce, getThis(), "connect", descriptor);

	/** 
	 * Database session settings initiated with each HTTP request. Oracle behaviour
	 * depends on particular NLS* parameter. Check if the developer has defined custom
	 * startup or create one from scratch
	 */
	if (phalcon_array_isset_fetch_str(&startup, descriptor, SL("startup")) && Z_TYPE(startup) == IS_ARRAY) {
		ZEND_HASH_FOREACH_VAL(Z_ARRVAL(startup), value) {
			PHALCON_CALL_METHOD(NULL, getThis(), "execute", value);
		} ZEND_HASH_FOREACH_END();
	}

	PHALCON_MM_RESTORE();
}

/**
 * Returns an array of Phalcon\Db\Column objects describing a table
 *
 * <code>print_r($connection->describeColumns("posts")); ?></code>
 *
 * @param string $table
 * @param string $schema
 * @return Phalcon\Db\Column[]
 */
PHP_METHOD(Phalcon_Db_Adapter_Pdo_Oracle, describeColumns){

	zval *table, *schema = NULL, columns, *dialect, sql, fetch_num;
	zval describe, old_column, *field;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 1, 1, &table, &schema);

	if (!schema) {
		schema = &PHALCON_GLOBAL(z_null);
	}

	array_init(&columns);

	dialect = phalcon_read_property(getThis(), SL("_dialect"), PH_NOISY);

	PHALCON_CALL_METHOD(&sql, dialect, "describecolumns", table, schema);

	/** 
	 * We're using FETCH_NUM to fetch the columns
	 */
	ZVAL_LONG(&fetch_num, PDO_FETCH_NUM);

	PHALCON_CALL_METHOD(&describe, getThis(), "fetchall", &sql, &fetch_num);

	/** 
	 *  0:column_name, 1:data_type, 2:data_length, 3:data_precision, 4:data_scale, 5:nullable, 6:constraint_type, 7:default, 8:position;
	 */
	ZEND_HASH_FOREACH_VAL(Z_ARRVAL(describe), field) {
		zval definition, column_size, column_precision, column_scale, column_type, attribute, column_name, column;

		array_init_size(&definition, 1);
		add_assoc_long_ex(&definition, SL("bindType"), 2);

		phalcon_array_fetch_long(&column_size, field, 2, PH_NOISY);
		phalcon_array_fetch_long(&column_precision, field, 3, PH_NOISY);
		phalcon_array_fetch_long(&column_scale, field, 4, PH_NOISY);
		phalcon_array_fetch_long(&column_type, field, 1, PH_NOISY);

		/** 
		 * Check the column type to get the correct Phalcon type
		 */
		while (1) {
			/**
			 * Integer
			 */
			if (phalcon_memnstr_str(&column_type, SL("NUMBER"))) {
				phalcon_array_update_str_long(&definition, SL("type"), PHALCON_DB_COLUMN_TYPE_DECIMAL, 0);
				phalcon_array_update_str_bool(&definition, SL("isNumeric"), 1, 0);
				phalcon_array_update_str(&definition, SL("size"), &column_precision, PH_COPY);
				phalcon_array_update_str_long(&definition, SL("bindType"), 32, 0);
				if (phalcon_is_numeric(&column_precision)) {
					phalcon_array_update_str_long(&definition, SL("bytes"), Z_LVAL(column_precision) * 8, 0);
				} else {
					phalcon_array_update_str_long(&definition, SL("size"), 30, 0);
					phalcon_array_update_str_long(&definition, SL("bytes"), 80, 0);
				}
				if (phalcon_is_numeric(&column_scale)) {
					phalcon_array_update_str(&definition, SL("scale"), &column_scale, PH_COPY);
				} else {
					phalcon_array_update_str_long(&definition, SL("scale"), 6, 0);
				}
				break;
			}

			/**
			 * Tinyint(1) is boolean
			 */
			if (phalcon_memnstr_str(&column_type, SL("TINYINT(1)"))) {
				phalcon_array_update_str_long(&definition, SL("type"), PHALCON_DB_COLUMN_TYPE_BOOLEAN, 0);
				phalcon_array_update_str_long(&definition, SL("bindType"), 5, 0);
				break;
			}

			/**
			 * Smallint/Bigint/Integers/Int are int
			 */
			if (phalcon_memnstr_str(&column_type, SL("INTEGER"))) {
				phalcon_array_update_str_long(&definition, SL("type"), PHALCON_DB_COLUMN_TYPE_INTEGER, 0);
				phalcon_array_update_str_bool(&definition, SL("isNumeric"), 1, 0);
				phalcon_array_update_str(&definition, SL("size"), &column_precision, PH_COPY);
				phalcon_array_update_str_long(&definition, SL("bindType"), 1, 0);
				phalcon_array_update_str_long(&definition, SL("bytes"), 32, 0);
				break;
			}

			/**
			 * Float/Smallfloats/Decimals are float
			 */
			if (phalcon_memnstr_str(&column_type, SL("FLOAT"))) {
				phalcon_array_update_str_long(&definition, SL("type"), PHALCON_DB_COLUMN_TYPE_FLOAT, 0);
				phalcon_array_update_str_bool(&definition, SL("isNumeric"), 1, 0);
				phalcon_array_update_str(&definition, SL("size"), &column_size, PH_COPY);
				phalcon_array_update_str(&definition, SL("scale"), &column_scale, PH_COPY);
				phalcon_array_update_str_long(&definition, SL("bindType"), 32, 0);
				break;
			}

			/**
			 * Date
			 */
			if (phalcon_memnstr_str(&column_type, SL("TIMESTAMP"))) {
				phalcon_array_update_str_long(&definition, SL("type"), PHALCON_DB_COLUMN_TYPE_DATE, 0);
				break;
			}

			/**
			 * Text
			 */
			if (phalcon_memnstr_str(&column_type, SL("RAW"))) {
				phalcon_array_update_str_long(&definition, SL("type"), PHALCON_DB_COLUMN_TYPE_TEXT, 0);
				break;
			}

			/**
			 * Text
			 */
			if (phalcon_memnstr_str(&column_type, SL("BLOB"))) {
				phalcon_array_update_str_long(&definition, SL("type"), PHALCON_DB_COLUMN_TYPE_TEXT, 0);
				break;
			}

			/**
			 * Text
			 */
			if (phalcon_memnstr_str(&column_type, SL("CLOB"))) {
				phalcon_array_update_str_long(&definition, SL("type"), PHALCON_DB_COLUMN_TYPE_TEXT, 0);
				break;
			}

			/**
			 * Chars2 are string
			 */
			if (phalcon_memnstr_str(&column_type, SL("VARCHAR2"))) {
				phalcon_array_update_str_long(&definition, SL("type"), PHALCON_DB_COLUMN_TYPE_VARCHAR, 0);
				phalcon_array_update_str(&definition, SL("size"), &column_size, PH_COPY);
				break;
			}

			/**
			 * Chars are chars
			 */
			if (phalcon_memnstr_str(&column_type, SL("CHAR"))) {
				phalcon_array_update_str_long(&definition, SL("type"), PHALCON_DB_COLUMN_TYPE_CHAR, 0);
				phalcon_array_update_str(&definition, SL("size"), &column_size, PH_COPY);
				break;
			}

			/**
			 * Text are varchars
			 */
			if (phalcon_memnstr_str(&column_type, SL("text"))) {
				phalcon_array_update_str_long(&definition, SL("type"), PHALCON_DB_COLUMN_TYPE_TEXT, 0);
				break;
			}

			/**
			 * By default is string
			 */
			phalcon_array_update_str_long(&definition, SL("type"), PHALCON_DB_COLUMN_TYPE_VARCHAR, 0);
			break;
		}

		if (!zend_is_true(&old_column)) {
			phalcon_array_update_str_bool(&definition, SL("first"), 1, 0);
		} else {
			phalcon_array_update_str(&definition, SL("after"), &old_column, PH_COPY);
		}

		/** 
		 * Check if the field is primary key
		 */
		phalcon_array_fetch_long(&attribute, field, 6, PH_NOISY);
		if (PHALCON_IS_STRING(&attribute, "P")) {
			phalcon_array_update_str_bool(&definition, SL("primary"), 1, 0);
		}

		/** 
		 * Check if the column allows null values
		 */
		phalcon_array_fetch_long(&attribute, field, 5, PH_NOISY);
		if (PHALCON_IS_STRING(&attribute, "N")) {
			phalcon_array_update_str_bool(&definition, SL("notNull"), 1, 0);
		}

		/** 
		 * If the column set the default values, get it
		 */
		phalcon_array_fetch_long(&attribute, field, 7, PH_NOISY);
		if (!PHALCON_IS_EMPTY(&attribute)) {
			phalcon_array_update_str(&definition, SL("default"), &attribute, PH_COPY);
		}

		phalcon_array_fetch_long(&column_name, field, 0, PH_NOISY);

		/** 
		 * Create a Phalcon\Db\Column to abstract the column
		 */
		object_init_ex(&column, phalcon_db_column_ce);
		PHALCON_CALL_METHOD(NULL, &column, "__construct", &column_name, &definition);

		phalcon_array_append(&columns, &column, PH_COPY);
		ZVAL_COPY_VALUE(&old_column, &column_name);
	} ZEND_HASH_FOREACH_END();

	RETURN_CTOR(&columns);
}

/**
 * Returns the insert id for the auto_increment/serial column inserted in the lastest executed SQL statement
 *
 *<code>
 * //Inserting a new robot
 * $success = $connection->insert(
 *     "robots",
 *     array("Astro Boy", 1952),
 *     array("name", "year")
 * );
 *
 * //Getting the generated id
 * $id = $connection->lastInsertId();
 *</code>
 *
 * @param string $sequenceName
 * @return int
 */
PHP_METHOD(Phalcon_Db_Adapter_Pdo_Oracle, lastInsertId){

	zval *sequence_name = NULL, sql, fetch_num, ret, insert_id;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 0, 1, &sequence_name);

	if (!sequence_name) {
		sequence_name = &PHALCON_GLOBAL(z_null);
	}

	PHALCON_CONCAT_SVS(&sql, "SELECT ", sequence_name, ".CURRVAL FROM dual");

	ZVAL_LONG(&fetch_num, PDO_FETCH_NUM);

	PHALCON_CALL_METHOD(&ret, getThis(), "fetchall", &sql, &fetch_num);

	phalcon_array_fetch_long(&insert_id, &ret, 0, PH_NOISY);
	RETURN_CTOR(&insert_id);
}

/**
 * Check whether the database system requires an explicit value for identity columns
 *
 * @return boolean
 */
PHP_METHOD(Phalcon_Db_Adapter_Pdo_Oracle, useExplicitIdValue){


	RETURN_FALSE;
}

/**
 * Return the default identity value to insert in an identity column
 *
 * @return Phalcon\Db\RawValue
 */
PHP_METHOD(Phalcon_Db_Adapter_Pdo_Oracle, getDefaultIdValue){

	zval null_value;

	PHALCON_MM_GROW();

	ZVAL_STRING(&null_value, "default");

	object_init_ex(return_value, phalcon_db_rawvalue_ce);
	PHALCON_CALL_METHOD(NULL, return_value, "__construct", &null_value);

	RETURN_MM();
}

/**
 * Check whether the database system requires a sequence to produce auto-numeric values
 *
 * @return boolean
 */
PHP_METHOD(Phalcon_Db_Adapter_Pdo_Oracle, supportSequences){


	RETURN_TRUE;
}

