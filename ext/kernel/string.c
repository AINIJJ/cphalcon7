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
  +------------------------------------------------------------------------+
*/

#include "kernel/string.h"

#include <ctype.h>
#include <Zend/zend_smart_str.h>
#include <ext/standard/php_string.h>
#include <ext/standard/php_rand.h>
#include <ext/standard/php_lcg.h>
#include <ext/standard/php_http.h>
#include <ext/standard/base64.h>
#include <ext/standard/md5.h>
#include <ext/standard/crc32.h>
#include <ext/standard/url.h>
#include <ext/standard/html.h>
#include <ext/date/php_date.h>

#ifdef PHALCON_USE_PHP_PCRE
#include <ext/pcre/php_pcre.h>
#endif

#ifdef PHALCON_USE_PHP_JSON
#include <ext/json/php_json.h>
#endif

#include "kernel/main.h"
#include "kernel/operators.h"
#include "kernel/fcall.h"

#define PH_RANDOM_ALNUM 0
#define PH_RANDOM_ALPHA 1
#define PH_RANDOM_HEXDEC 2
#define PH_RANDOM_NUMERIC 3
#define PH_RANDOM_NOZERO 4

/**
 * Fast call to php strlen
 */
void phalcon_fast_strlen(zval *return_value, zval *str){

	zval copy;
	int use_copy = 0;

	if (Z_TYPE_P(str) != IS_STRING) {
		use_copy = zend_make_printable_zval(str, &copy);
		if (use_copy) {
			str = &copy;
		}
	}

	ZVAL_LONG(return_value, Z_STRLEN_P(str));

	if (use_copy) {
		phalcon_ptr_dtor(str);
	}
}

/**
 * Fast call to php strlen
 */
int phalcon_fast_strlen_ev(zval *str){

	zval copy;
	int use_copy = 0, length;

	if (Z_TYPE_P(str) != IS_STRING) {
		use_copy = zend_make_printable_zval(str, &copy);
		if (use_copy) {
			str = &copy;
		}
	}

	length = Z_STRLEN_P(str);
	if (use_copy) {
		phalcon_ptr_dtor(str);
	}

	return length;
}

/**
 * Fast call to php strtolower
 */
void phalcon_fast_strtolower(zval *return_value, zval *str){

	zval copy;
	int use_copy = 0;
	char *lower_str;
	unsigned int length;

	if (Z_TYPE_P(str) != IS_STRING) {
		use_copy = zend_make_printable_zval(str, &copy);
		if (use_copy) {
			str = &copy;
		}
	}

	length = Z_STRLEN_P(str);
	lower_str = estrndup(Z_STRVAL_P(str), length);
	zend_str_tolower(lower_str, length);

	if (use_copy) {
		phalcon_ptr_dtor(str);
	}

	ZVAL_STRINGL(return_value, lower_str, length);
}

void phalcon_strtolower_inplace(zval *s) {
	if (likely(Z_TYPE_P(s) == IS_STRING)) {
		zend_str_tolower(Z_STRVAL_P(s), Z_STRLEN_P(s));
	}
}

/**
 * Fast call to php join  function
 */
void phalcon_fast_join(zval *result, zval *glue, zval *pieces){

	if (Z_TYPE_P(glue) != IS_STRING || Z_TYPE_P(pieces) != IS_ARRAY) {
		ZVAL_NULL(result);
		zend_error(E_WARNING, "Invalid arguments supplied for join()");
		return;
	}

	php_implode(Z_STR_P(glue), pieces, result);
}

/**
 * Appends to a smart_str a printable version of a zval
 */
void phalcon_append_printable_zval(smart_str *implstr, zval *tmp) {

	zval tmp_val;
	unsigned int str_len;

	switch (Z_TYPE_P(tmp)) {
		case IS_STRING:
			smart_str_appendl(implstr, Z_STRVAL_P(tmp), Z_STRLEN_P(tmp));
			break;

		case IS_LONG:
			smart_str_append_long(implstr, Z_LVAL_P(tmp));
			break;

		case IS_TRUE:
			smart_str_appendl(implstr, "1", sizeof("1") - 1);
			break;

		case IS_FALSE:
			break;

		case IS_NULL:
			break;

		case IS_DOUBLE: {
			char *stmp;
			str_len = spprintf(&stmp, 0, "%.*G", (int) EG(precision), Z_DVAL_P(tmp));
			smart_str_appendl(implstr, stmp, str_len);
			efree(stmp);
			break;
		}

		case IS_OBJECT: {
			zval expr;
			int copy = zend_make_printable_zval(tmp, &expr);
			smart_str_appendl(implstr, Z_STRVAL(expr), Z_STRLEN(expr));
			if (copy) {
				phalcon_dtor(expr);
			}
			break;
		}

		default:
			tmp_val = *tmp;
			zval_copy_ctor(&tmp_val);
			convert_to_string(&tmp_val);
			smart_str_appendl(implstr, Z_STRVAL(tmp_val), Z_STRLEN(tmp_val));
			phalcon_dtor(tmp_val);
			break;
	}
}

/**
 * Fast join function
 * This function is an adaption of the php_implode function
 *
 */
void phalcon_fast_join_str(zval *return_value, char *glue, unsigned int glue_length, zval *pieces){

	zval         *tmp;
	HashTable      *arr;
	smart_str      implstr = {0};
	unsigned int   numelems, i = 0;

	if (Z_TYPE_P(pieces) != IS_ARRAY) {
		php_error_docref(NULL, E_WARNING, "Invalid arguments supplied for fast_join()");
		RETURN_EMPTY_STRING();
	}

	arr = Z_ARRVAL_P(pieces);
	numelems = zend_hash_num_elements(arr);

	if (numelems == 0) {
		RETURN_EMPTY_STRING();
	}

	ZEND_HASH_FOREACH_VAL(arr, tmp) {
		phalcon_append_printable_zval(&implstr, tmp);
		if (++i != numelems) {
			smart_str_appendl(&implstr, glue, glue_length);
		}
	} ZEND_HASH_FOREACH_END();

	smart_str_0(&implstr);

	if (implstr.s->len) {
		RETURN_STR(implstr.s);
	} else {
		smart_str_free(&implstr);
		RETURN_EMPTY_STRING();
	}
}

/**
 * Convert dash/underscored texts returning camelized
 */
void phalcon_camelize(zval *return_value, const zval *str){

	int i, len;
	smart_str camelize_str = {0};
	char *marker, ch;

	if (unlikely(Z_TYPE_P(str) != IS_STRING)) {
		zend_error(E_WARNING, "Invalid arguments supplied for camelize()");
		RETURN_EMPTY_STRING();
		return;
	}

	marker = Z_STRVAL_P(str);
	len    = Z_STRLEN_P(str);

	for (i = 0; i < len; i++) {
		ch = marker[i];
		if (i == 0 || ch == '-' || ch == '_' || ch == '\\') {
			if (ch == '-' || ch == '_') {
				i++;
			} else if (ch == '\\') {
				smart_str_appendc(&camelize_str, marker[i]);
				i++;
			}

			if (i < len) {
				smart_str_appendc(&camelize_str, toupper(marker[i]));
			}
		}
		else {
			smart_str_appendc(&camelize_str, tolower(marker[i]));
		}
	}

	smart_str_0(&camelize_str);

	if (camelize_str.s) {
		RETURN_STR(camelize_str.s);
	} else {
		RETURN_EMPTY_STRING();
	}

}

/**
 * Convert dash/underscored texts returning camelized
 */
void phalcon_uncamelize(zval *return_value, const zval *str){

	int i;
	smart_str uncamelize_str = {0};
	char *marker, ch;

	if (Z_TYPE_P(str) != IS_STRING) {
		zend_error(E_WARNING, "Invalid arguments supplied for camelize()");
		return;
	}

	marker = Z_STRVAL_P(str);
	for (i = 0; i < Z_STRLEN_P(str); i++) {
		ch = *marker;
		if (ch == '\0') {
			break;
		}
		if (ch >= 'A' && ch <= 'Z') {
			if (i > 0) {
				smart_str_appendc(&uncamelize_str, '_');
			}
			smart_str_appendc(&uncamelize_str, (*marker) + 32);
		} else {
			smart_str_appendc(&uncamelize_str, (*marker));
		}
		marker++;
	}
	smart_str_0(&uncamelize_str);

	if (uncamelize_str.s) {
		RETURN_STR(uncamelize_str.s);
	} else {
		RETURN_EMPTY_STRING();
	}
}

/**
 * Fast call to explode php function
 */
void phalcon_fast_explode(zval *result, zval *delimiter, zval *str){

	if (unlikely(Z_TYPE_P(str) != IS_STRING || Z_TYPE_P(delimiter) != IS_STRING)) {
		ZVAL_NULL(result);
		zend_error(E_WARNING, "Invalid arguments supplied for explode()");
		return;
	}

	array_init(result);
	php_explode(Z_STR_P(delimiter), Z_STR_P(str), result, LONG_MAX);
}

/**
 * Fast call to explode php function
 */
void phalcon_fast_explode_str(zval *result, const char *delimiter, int delimiter_length, zval *str){

	if (unlikely(Z_TYPE_P(str) != IS_STRING)) {
		ZVAL_NULL(result);
		zend_error(E_WARNING, "Invalid arguments supplied for explode()");
		return;
	}

	array_init(result);
	php_explode(zend_string_init(delimiter, delimiter_length, 0), Z_STR_P(str), result, LONG_MAX);
}

/**
 * Check if a string is contained into another
 */
int phalcon_memnstr(const zval *haystack, const zval *needle) {

	if (Z_TYPE_P(haystack) != IS_STRING || Z_TYPE_P(needle) != IS_STRING) {
		zend_error(E_WARNING, "Invalid arguments supplied for memnstr()");
		return 0;
	}

	if (Z_STRLEN_P(haystack) >= Z_STRLEN_P(needle)) {
		return php_memnstr(Z_STRVAL_P(haystack), Z_STRVAL_P(needle), Z_STRLEN_P(needle), Z_STRVAL_P(haystack) + Z_STRLEN_P(haystack)) ? 1 : 0;
	}

	return 0;
}

/**
 * Check if a string is contained into another
 */
int phalcon_memnstr_str(const zval *haystack, char *needle, unsigned int needle_length) {

	if (Z_TYPE_P(haystack) != IS_STRING) {
		zend_error(E_WARNING, "Invalid arguments supplied for memnstr()");
		return 0;
	}

	if ((uint)(Z_STRLEN_P(haystack)) >= needle_length) {
		return php_memnstr(Z_STRVAL_P(haystack), needle, needle_length, Z_STRVAL_P(haystack) + Z_STRLEN_P(haystack)) ? 1 : 0;
	}

	return 0;
}

int phalcon_same_name(const char *key, const char *name, uint32_t name_len)
{
	char *lcname = zend_str_tolower_dup(name, name_len);
	int ret = memcmp(lcname, key, name_len) == 0;
	efree(lcname);
	return ret;
}

void phalcon_strtr(zval *return_value, zval *str, zval *str_from, zval *str_to) {

	if (Z_TYPE_P(str) != IS_STRING|| Z_TYPE_P(str_from) != IS_STRING|| Z_TYPE_P(str_to) != IS_STRING) {
		zend_error(E_WARNING, "Invalid arguments supplied for strtr()");
		return;
	}

	ZVAL_NEW_STR(return_value, Z_STR_P(str));

	php_strtr(Z_STRVAL_P(return_value),
			  Z_STRLEN_P(return_value),
			  Z_STRVAL_P(str_from),
			  Z_STRVAL_P(str_to),
			  MIN(Z_STRLEN_P(str_from),
			  Z_STRLEN_P(str_to)));
}

void phalcon_strtr_str(zval *return_value, zval *str, char *str_from, unsigned int str_from_length, char *str_to, unsigned int str_to_length) {

	if (Z_TYPE_P(str) != IS_STRING) {
		zend_error(E_WARNING, "Invalid arguments supplied for strtr()");
		return;
	}

	ZVAL_NEW_STR(return_value, Z_STR_P(str));

	php_strtr(Z_STRVAL_P(return_value),
			  Z_STRLEN_P(return_value),
			  str_from,
			  str_to,
			  MIN(str_from_length,
			  str_to_length));
}

/**
 * Inmediate function resolution for strpos function
 */
void phalcon_fast_strpos(zval *return_value, const zval *haystack, const zval *needle) {

	const char *found = NULL;

	if (unlikely(Z_TYPE_P(haystack) != IS_STRING || Z_TYPE_P(needle) != IS_STRING)) {
		ZVAL_NULL(return_value);
		zend_error(E_WARNING, "Invalid arguments supplied for strpos()");
		return;
	}

	if (!Z_STRLEN_P(needle)) {
		ZVAL_NULL(return_value);
		zend_error(E_WARNING, "Empty delimiter");
		return;
	}

	found = php_memnstr(Z_STRVAL_P(haystack), Z_STRVAL_P(needle), Z_STRLEN_P(needle), Z_STRVAL_P(haystack) + Z_STRLEN_P(haystack));

	if (found) {
		ZVAL_LONG(return_value, found-Z_STRVAL_P(haystack));
	} else {
		ZVAL_FALSE(return_value);
	}

}

/**
 * Inmediate function resolution for strpos function
 */
void phalcon_fast_strpos_str(zval *return_value, const zval *haystack, char *needle, unsigned int needle_length) {

	const char *found = NULL;

	if (unlikely(Z_TYPE_P(haystack) != IS_STRING)) {
		ZVAL_NULL(return_value);
		zend_error(E_WARNING, "Invalid arguments supplied for strpos()");
		return;
	}

	found = php_memnstr(Z_STRVAL_P(haystack), needle, needle_length, Z_STRVAL_P(haystack) + Z_STRLEN_P(haystack));

	if (found) {
		ZVAL_LONG(return_value, found-Z_STRVAL_P(haystack));
	} else {
		ZVAL_FALSE(return_value);
	}

}

/**
 * Inmediate function resolution for stripos function
 */
void phalcon_fast_stripos_str(zval *return_value, zval *haystack, char *needle, unsigned int needle_length) {

	const char *found = NULL;
	char *needle_dup, *haystack_dup;

	if (unlikely(Z_TYPE_P(haystack) != IS_STRING)) {
		ZVAL_NULL(return_value);
		zend_error(E_WARNING, "Invalid arguments supplied for stripos()");
		return;
	}

	haystack_dup = estrndup(Z_STRVAL_P(haystack), Z_STRLEN_P(haystack));
	zend_str_tolower(haystack_dup, Z_STRLEN_P(haystack));

	needle_dup = estrndup(needle, needle_length);
	zend_str_tolower(needle_dup, needle_length);

	found = php_memnstr(haystack_dup, needle, needle_length, haystack_dup + Z_STRLEN_P(haystack));

	efree(haystack_dup);
	efree(needle_dup);

	if (found) {
		ZVAL_LONG(return_value, found-Z_STRVAL_P(haystack));
	} else {
		ZVAL_FALSE(return_value);
	}

}

/**
 * Fast call to PHP trim() function
 */
zend_string* phalcon_trim(zval *str, zval *charlist, int where) {
	zval copy;
	zend_string *str;
	int use_copy = 0;

	if (Z_TYPE_P(str) != IS_STRING) {
		use_copy = zend_make_printable_zval(str, &copy);
	}

	if (charlist && Z_TYPE_P(charlist) != IS_STRING) {
		convert_to_string(charlist);
	}

	if (charlist) {
		str = php_trim(Z_STR_P(copy), Z_STRVAL_P(charlist), Z_STRLEN_P(charlist), where);
	} else {
		str = php_trim(Z_STR_P(copy), NULL, 0, where);
	}

	if (use_copy) {
		phalcon_dtor(copy);
	}

	return str;
}

/**
 * Fast call to PHP strip_tags() function
 */
void phalcon_fast_strip_tags(zval *return_value, zval *str) {

	zval copy;
	int use_copy = 0;
	char *stripped;
	size_t len;

	if (Z_TYPE_P(str) != IS_STRING) {
		use_copy = zend_make_printable_zval(str, &copy);
		if (use_copy) {
			str = &copy;
		}
	}

	stripped = estrndup(Z_STRVAL_P(str), Z_STRLEN_P(str));
	len = php_strip_tags(stripped, Z_STRLEN_P(str), NULL, NULL, 0);

	if (use_copy) {
		phalcon_dtor(copy);
	}

	ZVAL_STRINGL(return_value, stripped, len);
}

/**
 * Fast call to PHP strtoupper() function
 */
void phalcon_fast_strtoupper(zval *return_value, zval *str) {

	zval copy;
	int use_copy = 0;
	char *lower_str;
	unsigned int length;

	if (Z_TYPE_P(str) != IS_STRING) {
		use_copy = zend_make_printable_zval(str, &copy);
		if (use_copy) {
			str = &copy;
		}
	}

	length = Z_STRLEN_P(str);
	lower_str = estrndup(Z_STRVAL_P(str), length);
	php_strtoupper(lower_str, length);

	if (use_copy) {
		phalcon_ptr_dtor(str);
	}

	ZVAL_STRINGL(return_value, lower_str, length);
}

/**
 * Checks if a zval string starts with a zval string
 */
int phalcon_start_with(const zval *str, const zval *compared, zval *case_sensitive){

	int sensitive = 0;
	int i;
	char *op1_cursor, *op2_cursor;

	if (Z_TYPE_P(str) != IS_STRING || Z_TYPE_P(compared) != IS_STRING) {
		return 0;
	}

	if (!Z_STRLEN_P(compared) || !Z_STRLEN_P(str) || Z_STRLEN_P(compared) > Z_STRLEN_P(str)) {
		return 0;
	}

	if (case_sensitive) {
		sensitive = zend_is_true(case_sensitive);
	}

	if (!sensitive) {
		return !memcmp(Z_STRVAL_P(str), Z_STRVAL_P(compared), Z_STRLEN_P(compared));
	}

	op1_cursor = Z_STRVAL_P(str);
	op2_cursor = Z_STRVAL_P(compared);
	for (i = 0; i < Z_STRLEN_P(compared); i++) {
		if (tolower(*op1_cursor) != tolower(*op2_cursor)) {
			return 0;
		}

		op1_cursor++;
		op2_cursor++;
	}

	return 1;
}

/**
 * Checks if a zval string starts with a string
 */
int phalcon_start_with_str(const zval *str, char *compared, unsigned int compared_length){

	if (Z_TYPE_P(str) != IS_STRING || compared_length > (uint)(Z_STRLEN_P(str))) {
		return 0;
	}

	return !memcmp(Z_STRVAL_P(str), compared, compared_length);
}

/**
 * Checks if a string starts with other string
 */
int phalcon_start_with_str_str(char *str, unsigned int str_length, char *compared, unsigned int compared_length){

	if (compared_length > str_length) {
		return 0;
	}

	return !memcmp(str, compared, compared_length);
}

/**
 * Checks if a zval string ends with a zval string
 */
int phalcon_end_with(const zval *str, const zval *compared, zval *case_sensitive){

	int sensitive = 0;
	int i;
	char *op1_cursor, *op2_cursor;

	if (Z_TYPE_P(str) != IS_STRING || Z_TYPE_P(compared) != IS_STRING) {
		return 0;
	}

	if (!Z_STRLEN_P(compared) || !Z_STRLEN_P(str) || Z_STRLEN_P(compared) > Z_STRLEN_P(str)) {
		return 0;
	}

	if (case_sensitive) {
		sensitive = zend_is_true(case_sensitive);
	}

	if (!sensitive) {
		return !memcmp(Z_STRVAL_P(str) + Z_STRLEN_P(str) - Z_STRLEN_P(compared), Z_STRVAL_P(compared), Z_STRLEN_P(compared));
	}

	op1_cursor = Z_STRVAL_P(str) + Z_STRLEN_P(str) - Z_STRLEN_P(compared);
	op2_cursor = Z_STRVAL_P(compared);

	for (i = 0; i < Z_STRLEN_P(compared); ++i) {
		if (tolower(*op1_cursor) != tolower(*op2_cursor)) {
			return 0;
		}

		++op1_cursor;
		++op2_cursor;
	}

	return 1;
}

/**
 * Checks if a zval string ends with a *char string
 */
int phalcon_end_with_str(const zval *str, char *compared, unsigned int compared_length){

	if (Z_TYPE_P(str) != IS_STRING) {
		return 0;
	}

	if (!compared_length || !Z_STRLEN_P(str) || compared_length > (uint)(Z_STRLEN_P(str))) {
		return 0;
	}

	return !memcmp(Z_STRVAL_P(str) + Z_STRLEN_P(str) - compared_length, compared, compared_length);
}

/**
 * Checks if a zval string equal with other string
 */
int phalcon_comparestr(const zval *str, const zval *compared, zval *case_sensitive){

	if (Z_TYPE_P(str) != IS_STRING || Z_TYPE_P(compared) != IS_STRING) {
		return 0;
	}

	if (!Z_STRLEN_P(compared) || !Z_STRLEN_P(str) || Z_STRLEN_P(compared) != Z_STRLEN_P(str)) {
		return 0;
	}

	if (Z_STRVAL_P(str) == Z_STRVAL_P(compared)) {
		return 1;
	}

	if (!zend_is_true(case_sensitive)) {
		return !strcmp(Z_STRVAL_P(str), Z_STRVAL_P(compared));
	}

	return !strcasecmp(Z_STRVAL_P(str), Z_STRVAL_P(compared));
}

/**
 * Checks if a zval string equal with a zval string
 */
int phalcon_comparestr_str(const zval *str, char *compared, unsigned int compared_length, zval *case_sensitive){

	if (Z_TYPE_P(str) != IS_STRING) {
		return 0;
	}

	if (!compared_length || !Z_STRLEN_P(str) || compared_length != (uint)(Z_STRLEN_P(str))) {
		return 0;
	}

	if (!zend_is_true(case_sensitive)) {
		return !strcmp(Z_STRVAL_P(str), compared);
	}

	return !strcasecmp(Z_STRVAL_P(str), compared);
}

/**
 *
 */
void phalcon_random_string(zval *return_value, const zval *type, const zval *length) {

	long i, rand_type, ch;
	smart_str random_str = {0};

	if (Z_TYPE_P(type) != IS_LONG) {
		return;
	}

	if (Z_LVAL_P(type) > PH_RANDOM_NOZERO) {
		return;
	}

	if (Z_TYPE_P(length) != IS_LONG) {
		return;
	}

	/** Generate seed */
	if (!BG(mt_rand_is_seeded)) {
		php_mt_srand(GENERATE_SEED());
	}

	for (i = 0; i < Z_LVAL_P(length); i++) {

		switch (Z_LVAL_P(type)) {
			case PH_RANDOM_ALNUM:
				rand_type = (long) (php_mt_rand() >> 1);
				RAND_RANGE(rand_type, 0, 3, PHP_MT_RAND_MAX);
				break;
			case PH_RANDOM_ALPHA:
				rand_type = (long) (php_mt_rand() >> 1);
				RAND_RANGE(rand_type, 1, 2, PHP_MT_RAND_MAX);
				break;
			case PH_RANDOM_HEXDEC:
				rand_type = (long) (php_mt_rand() >> 1);
				RAND_RANGE(rand_type, 0, 1, PHP_MT_RAND_MAX);
				break;
			case PH_RANDOM_NUMERIC:
				rand_type = 0;
				break;
			case PH_RANDOM_NOZERO:
				rand_type = 5;
				break;
			default:
				continue;
		}

		switch (rand_type) {
			case 0:
				ch = (long) (php_mt_rand() >> 1);
				RAND_RANGE(ch, '0', '9', PHP_MT_RAND_MAX);
				break;
			case 1:
				ch = (long) (php_mt_rand() >> 1);
				RAND_RANGE(ch, 'a', 'f', PHP_MT_RAND_MAX);
				break;
			case 2:
				ch = (long) (php_mt_rand() >> 1);
				RAND_RANGE(ch, 'a', 'z', PHP_MT_RAND_MAX);
				break;
			case 3:
				ch = (long) (php_mt_rand() >> 1);
				RAND_RANGE(ch, 'A', 'Z', PHP_MT_RAND_MAX);
				break;
			case 5:
				ch = (long) (php_mt_rand() >> 1);
				RAND_RANGE(ch, '1', '9', PHP_MT_RAND_MAX);
				break;
			default:
				continue;
		}

		smart_str_appendc(&random_str, (unsigned int) ch);
	}


	smart_str_0(&random_str);

	if (random_str.s->len) {
		RETURN_STR(random_str.s);
	} else {
		smart_str_free(&random_str);
		RETURN_EMPTY_STRING();
	}

}

/**
 * Removes slashes at the end of a string
 */
void phalcon_remove_extra_slashes(zval *return_value, const zval *str) {

	char *cursor, *removed_str;
	unsigned int i;

	if (Z_TYPE_P(str) != IS_STRING) {
		RETURN_EMPTY_STRING();
	}

	if (Z_STRLEN_P(str) > 1) {
		cursor = Z_STRVAL_P(str);
		cursor += (Z_STRLEN_P(str) - 1);
		for (i = Z_STRLEN_P(str); i > 0; i--) {
			if ((*cursor) == '/') {
				cursor--;
				continue;
			}
			break;
		}
	} else {
		i = Z_STRLEN_P(str);
	}

    if (i <= Z_STRLEN_P(str)) {

    	removed_str = emalloc(i + 1);
    	memcpy(removed_str, Z_STRVAL_P(str), i);
    	removed_str[i] = '\0';

    	RETURN_STRINGL(removed_str, i);
    }

    RETURN_EMPTY_STRING();
}

/**
 * This function is not external in the Zend API so we redeclare it here in the extension
 */
int phalcon_spprintf(char **message, int max_len, char *format, ...)
{
    va_list arg;
    int len;

    va_start(arg, format);
    len = vspprintf(message, max_len, format, arg);
    va_end(arg);
    return len;
}

/**
 * Makes a substr like the PHP function. This function doesn't support negative lengths
 */
void phalcon_substr(zval *return_value, zval *str, unsigned long from, unsigned long length) {

	uint str_len = (uint)(Z_STRLEN_P(str));
	if (Z_TYPE_P(str) != IS_STRING) {

		if (Z_TYPE_P(str) == IS_NULL || PHALCON_IS_BOOL(str)) {
			RETURN_FALSE;
		}

		if (Z_TYPE_P(str) == IS_LONG) {
			RETURN_EMPTY_STRING();
		}

		zend_error(E_WARNING, "Invalid arguments supplied for phalcon_substr()");
		RETURN_FALSE;
	}

	if (str_len < from){
		RETURN_FALSE;
	}

	if (!length || (str_len < length + from)) {
		length = str_len - from;
	}

	if (!length){
		RETURN_EMPTY_STRING();
	}

	RETURN_STRINGL(Z_STRVAL_P(str) + from, (int)length);
}

void phalcon_append_printable_array(smart_str *implstr, zval *value) {

	zval         **tmp;
	HashTable      *arr;
	HashPosition   pos;
	unsigned int numelems, i = 0, str_len;

	arr = Z_ARRVAL_P(value);
	numelems = zend_hash_num_elements(arr);

	smart_str_appendc(implstr, '[');

	if (numelems > 0) {
		zend_hash_internal_pointer_reset_ex(arr, &pos);
		while ((tmp = zend_hash_get_current_data_ex(arr, &pos)) != NULL) {

			/**
			 * We don't serialize objects
			 */
			if (Z_TYPE_P(tmp) == IS_OBJECT) {
				smart_str_appendc(implstr, 'O');
				{
					char stmp[MAX_LENGTH_OF_LONG + 1];
					str_len = slprintf(stmp, sizeof(stmp), "%ld", Z_OBJ_HANDLE_P(tmp));
					smart_str_appendl(implstr, stmp, str_len);
				}
			} else {
				if (Z_TYPE_P(tmp) == IS_ARRAY) {
					phalcon_append_printable_array(implstr, tmp);
				} else {
					phalcon_append_printable_zval(implstr, tmp);
				}
			}

			if (++i != numelems) {
				smart_str_appendc(implstr, ',');
			}

			zend_hash_move_forward_ex(arr, &pos);
		}
	}

	smart_str_appendc(implstr, ']');
}

/**
 * Creates a unique key to be used as index in a hash
 */
void phalcon_unique_key(zval *return_value, zval *prefix, zval *value) {

	smart_str implstr = {0};

	if (Z_TYPE_P(prefix) == IS_STRING) {
		smart_str_appendl(&implstr, Z_STRVAL_P(prefix), Z_STRLEN_P(prefix));
	}

	if (Z_TYPE_P(value) == IS_ARRAY) {
		phalcon_append_printable_array(&implstr, value);
	} else {
		phalcon_append_printable_zval(&implstr, value);
	}

	smart_str_0(&implstr);

	if (implstr.len) {
		RETURN_STR(implstr.s);
	} else {
		smart_str_free(&implstr);
		RETURN_NULL();
	}
}

/**
 * Returns the PHP_EOL (if the passed parameter is TRUE)
 */
zval *phalcon_eol(int eol) {

	zval *local_eol;

	/**
	 * Initialize local var
	 */
	PHALCON_INIT_VAR(local_eol);

	/**
	 * Check if the eol is true and return PHP_EOL or empty string
	 */
	if (eol) {
		ZVAL_STRING(local_eol, PHP_EOL);
	} else {
		ZVAL_EMPTY_STRING(local_eol);
	}

	return local_eol;
}

/**
 * Base 64 encode
 */
void phalcon_base64_encode(zval *return_value, zval *data) {

	zval copy;
	char *encoded;
	int use_copy = 0, length;

	if (Z_TYPE_P(data) != IS_STRING) {
		use_copy = zend_make_printable_zval(data, &copy);
		if (use_copy) {
			data = &copy;
		}
	}

	encoded = (char *) php_base64_encode((unsigned char *)(Z_STRVAL_P(data)), Z_STRLEN_P(data), &length);

	if (use_copy) {
		phalcon_dtor(data);
	}

	if (encoded) {
		RETURN_STRINGL(encoded, length);
	} else {
		RETURN_NULL();
	}
}

/**
 * Base 64 decode
 */
void phalcon_base64_decode(zval *return_value, zval *data) {

	zval copy;
	char *decoded;
	int use_copy = 0, length;

	if (Z_TYPE_P(data) != IS_STRING) {
		use_copy = zend_make_printable_zval(data, &copy);
		if (use_copy) {
			data = &copy;
		}
	}

	decoded = (char *) php_base64_decode((unsigned char *)(Z_STRVAL_P(data)), Z_STRLEN_P(data), &length);

	if (use_copy) {
		phalcon_dtor(data);
	}

	if (decoded) {
		RETURN_STRINGL(decoded, length);
	} else {
		RETURN_NULL();
	}
}

void phalcon_md5(zval *return_value, zval *str) {

	PHP_MD5_CTX ctx;
	unsigned char digest[16];
	char hexdigest[33];
	zval copy;
	int use_copy = 0;

	if (Z_TYPE_P(str) != IS_STRING) {
		use_copy = zend_make_printable_zval(str, &copy);
		if (use_copy) {
			str = &copy;
		}
	}

	PHP_MD5Init(&ctx);
	PHP_MD5Update(&ctx, Z_STRVAL_P(str), Z_STRLEN_P(str));
	PHP_MD5Final(digest, &ctx);

	make_digest(hexdigest, digest);

	ZVAL_STRINGL(return_value, hexdigest, 32);
}

void phalcon_crc32(zval *return_value, zval *str) {

	zval copy;
	int use_copy = 0;
	size_t nr;
	char *p;
	php_uint32 crc;
	php_uint32 crcinit = 0;

	if (Z_TYPE_P(str) != IS_STRING) {
		use_copy = zend_make_printable_zval(str, &copy);
		if (use_copy) {
			str = &copy;
		}
	}

	p = Z_STRVAL_P(str);
	nr = Z_STRLEN_P(str);

	crc = crcinit^0xFFFFFFFF;
	for (; nr--; ++p) {
		crc = ((crc >> 8) & 0x00FFFFFF) ^ crc32tab[(crc ^ (*p)) & 0xFF];
	}

	if (use_copy) {
		phalcon_dtor(str);
	}

	RETVAL_LONG(crc ^ 0xFFFFFFFF);
}

int phalcon_preg_match(zval *return_value, zval *regex, zval *subject, zval *matches)
{
	zval *params[] = { regex, subject, matches };
	int result;

	if (matches) {
		Z_SET_ISREF_P(matches);
	}

	result = phalcon_call_func_aparams(return_value, SL("preg_match"), (matches ? 3 : 2), params);

	if (matches) {
		Z_UNSET_ISREF_P(matches);
	}

	return result;
}

int phalcon_json_encode(zval *return_value, zval *v, int opts)
{
	zval *zopts;
	zval *params[2];
	int result;

	PHALCON_ALLOC_GHOST_ZVAL(zopts);
	ZVAL_LONG(zopts, opts);

	params[0] = v;
	params[1] = zopts;
	result = phalcon_call_func_aparams(return_value, ZEND_STRL("json_encode"), 2, params);

	phalcon_ptr_dtor(zopts);
	return result;
}

int phalcon_json_decode(zval *return_value, zval *v, zend_bool assoc)
{
	zval *zassoc = assoc ? &PHALCON_GLOBAL(z_true) : &PHALCON_GLOBAL(z_false);
	zval *params[] = { v, zassoc };

	return phalcon_call_func_aparams(return_value, ZEND_STRL("json_decode"), 2, params);
}

void phalcon_lcfirst(zval *return_value, zval *s)
{
	zval copy;
	char *c;
	int use_copy = 0;

	if (unlikely(Z_TYPE_P(s) != IS_STRING)) {
		use_copy = zend_make_printable_zval(s, &copy);
		if (use_copy) {
			s = &copy;
		}
	}

	if (!Z_STRLEN_P(s)) {
		ZVAL_EMPTY_STRING(return_value);
	}
	else {
		ZVAL_NEW_STR(return_value, Z_STR_P(s));
		c = Z_STRVAL_P(return_value);
		*c = tolower((unsigned char)*c);
	}

	if (unlikely(use_copy)) {
		phalcon_dtor(copy);
	}
}

void phalcon_ucfirst(zval *return_value, zval *s)
{
	zval copy;
	char *c;
	int use_copy = 0;

	if (unlikely(Z_TYPE_P(s) != IS_STRING)) {
		use_copy = zend_make_printable_zval(s, &copy);
		if (use_copy) {
			s = &copy;
		}
	}

	if (!Z_STRLEN_P(s)) {
		ZVAL_EMPTY_STRING(return_value);
	}
	else {
		ZVAL_NEW_STR(return_value, Z_STR_P(s));
		c = Z_STRVAL_P(return_value);
		*c = toupper((unsigned char)*c);
	}

	if (unlikely(use_copy)) {
		phalcon_dtor(copy);
	}
}

int phalcon_http_build_query(zval *return_value, zval *params, char *sep)
{
	if (Z_TYPE_P(params) == IS_ARRAY || Z_TYPE_P(params) == IS_OBJECT) {
		smart_str formstr = { NULL, 0, 0 };
		int res;

		res = php_url_encode_hash_ex(HASH_OF(params), &formstr, NULL, 0, NULL, 0, NULL, 0, (Z_TYPE_P(params) == IS_OBJECT ? params : NULL), sep, PHP_QUERY_RFC1738);

		if (res == SUCCESS) {
			if (!formstr.s) {
				ZVAL_EMPTY_STRING(return_value);
			} else {
				smart_str_0(&formstr);
				ZVAL_STR(return_value, formstr.s);
			}

			return SUCCESS;
		}

		smart_str_free(&formstr);
		ZVAL_FALSE(return_value);
	}
	else {
		ZVAL_NULL(return_value);
	}

	return FAILURE;
}

void phalcon_htmlspecialchars(zval *return_value, zval *string, zval *quoting, zval *charset)
{
	zval copy;
	zend_string *escaped;
	char *cs;
	int qs, use_copy = 0;
	size_t escaped_len;

	if (unlikely(Z_TYPE_P(string) != IS_STRING)) {
		use_copy = zend_make_printable_zval(string, &copy);
		if (use_copy) {
			string = &copy;
		}
	}

	cs = (charset && Z_TYPE_P(charset) == IS_STRING) ? Z_STRVAL_P(charset) : NULL;
	qs = (quoting && Z_TYPE_P(quoting) == IS_LONG)   ? Z_LVAL_P(quoting)   : ENT_COMPAT;

	escaped = php_escape_html_entities_ex((unsigned char *)(Z_STRVAL_P(string)), Z_STRLEN_P(string), 0, qs, cs, 1);
	ZVAL_STR(return_value, escaped);

	if (unlikely(use_copy)) {
		phalcon_dtor(copy);
	}
}

void phalcon_htmlentities(zval *return_value, zval *string, zval *quoting, zval *charset)
{
	zval copy;
	zend_string *escaped;
	char *cs;
	int qs, use_copy = 0;
	size_t escaped_len;

	if (unlikely(Z_TYPE_P(string) != IS_STRING)) {
		use_copy = zend_make_printable_zval(string, &copy);
		if (use_copy) {
			string = &copy;
		}
	}

	cs = (charset && Z_TYPE_P(charset) == IS_STRING) ? Z_STRVAL_P(charset) : NULL;
	qs = (quoting && Z_TYPE_P(quoting) == IS_LONG)   ? Z_LVAL_P(quoting)   : ENT_COMPAT;

	escaped = php_escape_html_entities_ex((unsigned char *)(Z_STRVAL_P(string)), Z_STRLEN_P(string), 1, qs, cs, 1);
	ZVAL_STR(return_value, escaped);

	if (unlikely(use_copy)) {
		phalcon_dtor(copy);
	}
}

void phalcon_strval(zval *return_value, zval *v)
{
	zval copy;
	int use_copy = 0;

	use_copy = zend_make_printable_zval(v, &copy);
	if (use_copy) {
		zval *tmp = &copy;
		ZVAL_ZVAL(return_value, tmp, 0, 0);
	}
	else {
		ZVAL_ZVAL(return_value, v, 1, 0);
	}
}

void phalcon_date(zval *return_value, zval *format, zval *timestamp)
{
	long int ts;
	zval copy;
	int use_copy = 0;
	char *formatted;

	if (unlikely(Z_TYPE_P(format) != IS_STRING)) {
		use_copy = zend_make_printable_zval(format, &copy);
		if (use_copy) {
			format = &copy;
		}
	}

	ts = (timestamp) ? phalcon_get_intval(timestamp) : time(NULL);

	formatted = php_format_date(Z_STRVAL_P(format), Z_STRLEN_P(format), ts, 1);
	ZVAL_STRING(return_value, formatted);

	if (unlikely(use_copy)) {
		phalcon_dtor(copy);
	}
}

void phalcon_addslashes(zval *return_value, zval *str)
{
	zval copy;
	int use_copy = 0;

	if (unlikely(Z_TYPE_P(str) != IS_STRING)) {
		use_copy = zend_make_printable_zval(str, &copy);
		if (use_copy) {
			str = &copy;
		}
	}

	ZVAL_STRING(return_value, php_addslashes(Z_STRVAL_P(str), Z_STRLEN_P(str), &Z_STRLEN_P(return_value), 0));

	if (unlikely(use_copy)) {
		phalcon_dtor(copy);
	}
}

void phalcon_add_trailing_slash(zval** v)
{
	PHALCON_ENSURE_IS_STRING(v);
	if (Z_STRLEN_P(*v)) {
		int len = Z_STRLEN_P(*v);
		char *c = Z_STRVAL_P(*v);

#ifdef PHP_WIN32
		if (c[len - 1] != '/' && c[len - 1] != '\\')
#else
		if (c[len - 1] != PHP_DIR_SEPARATOR)
#endif
		{            
			SEPARATE_ZVAL(v);
			c = Z_STRVAL_P(*v);

			if (!STR_IS_INTERNED(c)) {
				c = erealloc(c, len+2);
			}
			else {
				c = emalloc(len + 2);
				if (c != NULL) {
					memcpy(c, Z_STRVAL_P(*v), Z_STRLEN_P(*v));
				}
			}

			if (c != NULL) {
				c[len]   = PHP_DIR_SEPARATOR;
				c[len + 1] = 0;

				ZVAL_STRINGL(*v, c, len+1);
			}
		}
	}
}

void phalcon_stripslashes(zval *return_value, zval *str)
{
	zval copy;
	int use_copy = 0;

	if (unlikely(Z_TYPE_P(str) != IS_STRING)) {
		use_copy = zend_make_printable_zval(str, &copy);
		if (use_copy) {
			str = &copy;
		}
	}

	ZVAL_NEW_STR(return_value, Z_STR_P(str));
	php_stripslashes(Z_STR_P(return_value));

	if (unlikely(use_copy)) {
		phalcon_dtor(copy);
	}
}

void phalcon_stripcslashes(zval *return_value, zval *str)
{

	zval copy;
	int use_copy = 0;

	if (unlikely(Z_TYPE_P(str) != IS_STRING)) {
		use_copy = zend_make_printable_zval(str, &copy);
		if (use_copy) {
			str = &copy;
		}
	}

	ZVAL_NEW_STR(return_value, Z_STR_P(str));
	php_stripcslashes(Z_STR_P(return_value));

	if (unlikely(use_copy)) {
		phalcon_dtor(copy);
	}
}
