
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
  |          Rack Lin <racklin@gmail.com>                                  |
  +------------------------------------------------------------------------+
*/

#include "dispatcher.h"
#include "dispatcherinterface.h"
#include "diinterface.h"
#include "di/injectable.h"
#include "events/eventsawareinterface.h"
#include "exception.h"
#include "filterinterface.h"

#include "kernel/main.h"
#include "kernel/memory.h"
#include "kernel/object.h"
#include "kernel/fcall.h"
#include "kernel/array.h"
#include "kernel/concat.h"
#include "kernel/operators.h"
#include "kernel/string.h"
#include "kernel/exception.h"

#include "interned-strings.h"

/**
 * Phalcon\Dispatcher
 *
 * This is the base class for Phalcon\Mvc\Dispatcher and Phalcon\CLI\Dispatcher.
 * This class can't be instantiated directly, you can use it to create your own dispatchers
 */
zend_class_entry *phalcon_dispatcher_ce;

PHP_METHOD(Phalcon_Dispatcher, __construct);
PHP_METHOD(Phalcon_Dispatcher, setActionSuffix);
PHP_METHOD(Phalcon_Dispatcher, setModuleName);
PHP_METHOD(Phalcon_Dispatcher, getModuleName);
PHP_METHOD(Phalcon_Dispatcher, setNamespaceName);
PHP_METHOD(Phalcon_Dispatcher, getNamespaceName);
PHP_METHOD(Phalcon_Dispatcher, setDefaultNamespace);
PHP_METHOD(Phalcon_Dispatcher, getDefaultNamespace);
PHP_METHOD(Phalcon_Dispatcher, setDefaultAction);
PHP_METHOD(Phalcon_Dispatcher, setActionName);
PHP_METHOD(Phalcon_Dispatcher, getActionName);
PHP_METHOD(Phalcon_Dispatcher, setParams);
PHP_METHOD(Phalcon_Dispatcher, getParams);
PHP_METHOD(Phalcon_Dispatcher, setParam);
PHP_METHOD(Phalcon_Dispatcher, getParam);
PHP_METHOD(Phalcon_Dispatcher, getActiveMethod);
PHP_METHOD(Phalcon_Dispatcher, isFinished);
PHP_METHOD(Phalcon_Dispatcher, setReturnedValue);
PHP_METHOD(Phalcon_Dispatcher, getReturnedValue);
PHP_METHOD(Phalcon_Dispatcher, dispatch);
PHP_METHOD(Phalcon_Dispatcher, forward);
PHP_METHOD(Phalcon_Dispatcher, wasForwarded);
PHP_METHOD(Phalcon_Dispatcher, getHandlerClass);
PHP_METHOD(Phalcon_Dispatcher, camelizeNamespace);
PHP_METHOD(Phalcon_Dispatcher, camelizeController);
PHP_METHOD(Phalcon_Dispatcher, setErrorHandler);
PHP_METHOD(Phalcon_Dispatcher, getErrorHandler);
PHP_METHOD(Phalcon_Dispatcher, fireEvent);

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_dispatcher_setmodulename, 0, 0, 1)
	ZEND_ARG_INFO(0, moduleName)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_dispatcher_setnamespacename, 0, 0, 1)
	ZEND_ARG_INFO(0, namespaceName)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_dispatcher_setreturnedvalue, 0, 0, 1)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

static const zend_function_entry phalcon_dispatcher_method_entry[] = {
	PHP_ME(Phalcon_Dispatcher, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(Phalcon_Dispatcher, setActionSuffix, arginfo_phalcon_dispatcherinterface_setactionsuffix, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Dispatcher, setModuleName, arginfo_phalcon_dispatcher_setmodulename, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Dispatcher, getModuleName, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Dispatcher, setNamespaceName, arginfo_phalcon_dispatcher_setnamespacename, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Dispatcher, getNamespaceName, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Dispatcher, setDefaultNamespace, arginfo_phalcon_dispatcherinterface_setdefaultnamespace, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Dispatcher, getDefaultNamespace, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Dispatcher, setDefaultAction, arginfo_phalcon_dispatcherinterface_setdefaultaction, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Dispatcher, setActionName, arginfo_phalcon_dispatcherinterface_setactionname, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Dispatcher, getActionName, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Dispatcher, setParams, arginfo_phalcon_dispatcherinterface_setparams, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Dispatcher, getParams, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Dispatcher, setParam, arginfo_phalcon_dispatcherinterface_setparam, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Dispatcher, getParam, arginfo_phalcon_dispatcherinterface_getparam, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Dispatcher, getActiveMethod, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Dispatcher, isFinished, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Dispatcher, setReturnedValue, arginfo_phalcon_dispatcher_setreturnedvalue, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Dispatcher, getReturnedValue, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Dispatcher, dispatch, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Dispatcher, forward, arginfo_phalcon_dispatcherinterface_forward, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Dispatcher, wasForwarded, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Dispatcher, getHandlerClass, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Dispatcher, camelizeNamespace, arginfo_phalcon_dispatcherinterface_camelizenamespace, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Dispatcher, camelizeController, arginfo_phalcon_dispatcherinterface_camelizecontroller, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Dispatcher, setErrorHandler, arginfo_phalcon_dispatcherinterface_seterrorhandler, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Dispatcher, getErrorHandler, arginfo_phalcon_dispatcherinterface_geterrorhandler, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Dispatcher, fireEvent, arginfo_phalcon_di_injectable_fireevent, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

/**
 * Phalcon\Dispatcher initializer
 */
PHALCON_INIT_CLASS(Phalcon_Dispatcher){

	PHALCON_REGISTER_CLASS_EX(Phalcon, Dispatcher, dispatcher, phalcon_di_injectable_ce, phalcon_dispatcher_method_entry, ZEND_ACC_EXPLICIT_ABSTRACT_CLASS);

	zend_declare_property_null(phalcon_dispatcher_ce, SL("_activeHandler"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_dispatcher_ce, SL("_finished"), ZEND_ACC_PROTECTED);
	zend_declare_property_bool(phalcon_dispatcher_ce, SL("_forwarded"), 0, ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_dispatcher_ce, SL("_moduleName"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_dispatcher_ce, SL("_namespaceName"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_dispatcher_ce, SL("_handlerName"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_dispatcher_ce, SL("_actionName"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_dispatcher_ce, SL("_params"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_dispatcher_ce, SL("_returnedValue"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_dispatcher_ce, SL("_lastHandler"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_dispatcher_ce, SL("_defaultNamespace"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_dispatcher_ce, SL("_defaultHandler"), ZEND_ACC_PROTECTED);
	zend_declare_property_string(phalcon_dispatcher_ce, SL("_defaultAction"), "", ZEND_ACC_PROTECTED);
	zend_declare_property_string(phalcon_dispatcher_ce, SL("_handlerSuffix"), "", ZEND_ACC_PROTECTED);
	zend_declare_property_string(phalcon_dispatcher_ce, SL("_actionSuffix"), "Action", ZEND_ACC_PROTECTED);
	zend_declare_property_bool(phalcon_dispatcher_ce, SL("_isExactHandler"), 0, ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_dispatcher_ce, SL("_previousNamespaceName"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_dispatcher_ce, SL("_previousHandlerName"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_dispatcher_ce, SL("_previousActionName"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_dispatcher_ce, SL("_previousParams"), ZEND_ACC_PROTECTED);
	zend_declare_property_bool(phalcon_dispatcher_ce, SL("_camelizeNamespace"), 1, ZEND_ACC_PROTECTED);
	zend_declare_property_bool(phalcon_dispatcher_ce, SL("_camelizeController"), 1, ZEND_ACC_PROTECTED);
	zend_declare_property_bool(phalcon_dispatcher_ce, SL("_errorHandlers"), 1, ZEND_ACC_PROTECTED);

	zend_declare_class_constant_long(phalcon_dispatcher_ce, SL("EXCEPTION_NO_DI"), PHALCON_EXCEPTION_NO_DI);
	zend_declare_class_constant_long(phalcon_dispatcher_ce, SL("EXCEPTION_CYCLIC_ROUTING"), PHALCON_EXCEPTION_CYCLIC_ROUTING);
	zend_declare_class_constant_long(phalcon_dispatcher_ce, SL("EXCEPTION_HANDLER_NOT_FOUND"), PHALCON_EXCEPTION_HANDLER_NOT_FOUND);
	zend_declare_class_constant_long(phalcon_dispatcher_ce, SL("EXCEPTION_INVALID_HANDLER"), PHALCON_EXCEPTION_INVALID_HANDLER);
	zend_declare_class_constant_long(phalcon_dispatcher_ce, SL("EXCEPTION_INVALID_PARAMS"), PHALCON_EXCEPTION_INVALID_PARAMS);
	zend_declare_class_constant_long(phalcon_dispatcher_ce, SL("EXCEPTION_ACTION_NOT_FOUND"), PHALCON_EXCEPTION_ACTION_NOT_FOUND);

	zend_class_implements(phalcon_dispatcher_ce, 1, phalcon_dispatcherinterface_ce);

	return SUCCESS;
}

/**
 * Phalcon\Dispatcher constructor
 */
PHP_METHOD(Phalcon_Dispatcher, __construct){

	phalcon_update_property_empty_array(this_ptr, SL("_params"));
}

/**
 * Sets the dependency injector
 *
 * @param Phalcon\DiInterface $dependencyInjector
 */
PHP_METHOD(Phalcon_Dispatcher, setDI){

	zval *dependency_injector;

	phalcon_fetch_params(0, 1, 0, &dependency_injector);
	PHALCON_VERIFY_INTERFACE_EX(dependency_injector, phalcon_diinterface_ce, phalcon_exception_ce, 0);
	phalcon_update_property_this(this_ptr, SL("_dependencyInjector"), dependency_injector);
}

/**
 * Returns the internal dependency injector
 *
 * @return Phalcon\DiInterface
 */
PHP_METHOD(Phalcon_Dispatcher, getDI){


	RETURN_MEMBER(this_ptr, "_dependencyInjector");
}

/**
 * Sets the events manager
 *
 * @param Phalcon\Events\ManagerInterface $eventsManager
 */
PHP_METHOD(Phalcon_Dispatcher, setEventsManager){

	zval *events_manager;

	phalcon_fetch_params(0, 1, 0, &events_manager);

	phalcon_update_property_this(this_ptr, SL("_eventsManager"), events_manager);

}

/**
 * Returns the internal event manager
 *
 * @return Phalcon\Events\ManagerInterface
 */
PHP_METHOD(Phalcon_Dispatcher, getEventsManager){


	RETURN_MEMBER(this_ptr, "_eventsManager");
}

/**
 * Sets the default action suffix
 *
 * @param string $actionSuffix
 */
PHP_METHOD(Phalcon_Dispatcher, setActionSuffix){

	zval *action_suffix;

	phalcon_fetch_params(0, 1, 0, &action_suffix);

	phalcon_update_property_this(this_ptr, SL("_actionSuffix"), action_suffix);

}

/**
 * Sets the module where the controller is (only informative)
 *
 * @param string $moduleName
 */
PHP_METHOD(Phalcon_Dispatcher, setModuleName){

	zval *module_name;

	phalcon_fetch_params(0, 1, 0, &module_name);

	phalcon_update_property_this(this_ptr, SL("_moduleName"), module_name);

}

/**
 * Gets the module where the controller class is
 *
 * @return string
 */
PHP_METHOD(Phalcon_Dispatcher, getModuleName){


	RETURN_MEMBER(this_ptr, "_moduleName");
}

/**
 * Sets the namespace where the controller class is
 *
 * @param string $namespaceName
 */
PHP_METHOD(Phalcon_Dispatcher, setNamespaceName){

	zval *namespace_name;

	phalcon_fetch_params(0, 1, 0, &namespace_name);

	phalcon_update_property_this(this_ptr, SL("_namespaceName"), namespace_name);

}

/**
 * Gets a namespace to be prepended to the current handler name
 *
 * @return string
 */
PHP_METHOD(Phalcon_Dispatcher, getNamespaceName){


	RETURN_MEMBER(this_ptr, "_namespaceName");
}

/**
 * Sets the default namespace
 *
 * @param string $namespace
 */
PHP_METHOD(Phalcon_Dispatcher, setDefaultNamespace){

	zval *namespace;

	phalcon_fetch_params(0, 1, 0, &namespace);

	phalcon_update_property_this(this_ptr, SL("_defaultNamespace"), namespace);

}

/**
 * Returns the default namespace
 *
 * @return string
 */
PHP_METHOD(Phalcon_Dispatcher, getDefaultNamespace){


	RETURN_MEMBER(this_ptr, "_defaultNamespace");
}

/**
 * Sets the default action name
 *
 * @param string $actionName
 */
PHP_METHOD(Phalcon_Dispatcher, setDefaultAction){

	zval *action_name;

	phalcon_fetch_params(0, 1, 0, &action_name);

	phalcon_update_property_this(this_ptr, SL("_defaultAction"), action_name);

}

/**
 * Sets the action name to be dispatched
 *
 * @param string $actionName
 */
PHP_METHOD(Phalcon_Dispatcher, setActionName){

	zval *action_name;

	phalcon_fetch_params(0, 1, 0, &action_name);

	phalcon_update_property_this(this_ptr, SL("_actionName"), action_name);

}

/**
 * Gets the lastest dispatched action name
 *
 * @return string
 */
PHP_METHOD(Phalcon_Dispatcher, getActionName){


	RETURN_MEMBER(this_ptr, "_actionName");
}

/**
 * Sets action params to be dispatched
 *
 * @param array $params
 */
PHP_METHOD(Phalcon_Dispatcher, setParams){

	zval *params, *exception_code, *exception_message;

	phalcon_fetch_params(0, 1, 0, &params);

	if (Z_TYPE_P(params) != IS_ARRAY) {
		PHALCON_MM_GROW();

		PHALCON_INIT_VAR(exception_code);
		ZVAL_LONG(exception_code, PHALCON_EXCEPTION_INVALID_PARAMS);

		PHALCON_INIT_VAR(exception_message);
		ZVAL_STRING(exception_message, "Parameters must be an Array");
		PHALCON_CALL_METHOD(NULL, this_ptr, "_throwdispatchexception", exception_message, exception_code);
		RETURN_MM_NULL();
	}

	phalcon_update_property_this(this_ptr, SL("_params"), params);
}

/**
 * Gets action params
 *
 * @return array
 */
PHP_METHOD(Phalcon_Dispatcher, getParams){


	RETURN_MEMBER(this_ptr, "_params");
}

/**
 * Set a param by its name or numeric index
 *
 * @param  mixed $param
 * @param  mixed $value
 */
PHP_METHOD(Phalcon_Dispatcher, setParam){

	zval *param, *value;

	phalcon_fetch_params(0, 2, 0, &param, &value);

	phalcon_update_property_array(this_ptr, SL("_params"), param, value);

}

/**
 * Gets a param by its name or numeric index
 *
 * @param  mixed $param
 * @param  string|array $filters
 * @param  mixed $defaultValue
 * @return mixed
 */
PHP_METHOD(Phalcon_Dispatcher, getParam){

	zval *param, *filters = NULL, *default_value = NULL;
	zval *exception_code;
	zval *exception_message, *service, *filter = NULL;
	zval *params, *param_value, *dependency_injector;

	phalcon_fetch_params(0, 1, 2, &param, &filters, &default_value);

	params = phalcon_read_property(this_ptr, SL("_params"), PH_NOISY);
	if (phalcon_array_isset_fetch(&param_value, params, param)) {

		if (filters && Z_TYPE_P(filters) != IS_NULL) {
			PHALCON_MM_GROW();
			dependency_injector = phalcon_read_property(this_ptr, SL("_dependencyInjector"), PH_NOISY);
			if (Z_TYPE_P(dependency_injector) != IS_OBJECT) {
				PHALCON_INIT_VAR(exception_code);
				ZVAL_LONG(exception_code, PHALCON_EXCEPTION_NO_DI);

				PHALCON_INIT_VAR(exception_message);
				ZVAL_STRING(exception_message, "A dependency injection object is required to access the 'filter' service");
				PHALCON_CALL_METHOD(NULL, this_ptr, "_throwdispatchexception", exception_message, exception_code);
				RETURN_MM();
			}

			PHALCON_INIT_VAR(service);
			PHALCON_ZVAL_MAYBE_INTERNED_STRING(service, phalcon_interned_filter);

			PHALCON_CALL_METHOD(&filter, dependency_injector, "getshared", service);
			PHALCON_VERIFY_INTERFACE(filter, phalcon_filterinterface_ce);
			PHALCON_RETURN_CALL_METHOD(filter, "sanitize", param_value, filters);
			RETURN_MM();
		} else {
			RETURN_ZVAL(param_value, 1, 0);
		}
	}

	if (default_value) {
		RETURN_ZVAL(default_value, 1, 0);
	}

	RETURN_NULL();
}

/**
 * Returns the current method to be/executed in the dispatcher
 *
 * @return string
 */
PHP_METHOD(Phalcon_Dispatcher, getActiveMethod){

	zval *suffix, *action_name;

	suffix      = phalcon_read_property(this_ptr, SL("_actionSuffix"), PH_NOISY);
	action_name = phalcon_read_property(this_ptr, SL("_actionName"), PH_NOISY);

	PHALCON_CONCAT_VV(return_value, action_name, suffix);
}

/**
 * Checks if the dispatch loop is finished or has more pendent controllers/tasks to disptach
 *
 * @return boolean
 */
PHP_METHOD(Phalcon_Dispatcher, isFinished){


	RETURN_MEMBER(this_ptr, "_finished");
}

/**
 * Sets the latest returned value by an action manually
 *
 * @param mixed $value
 */
PHP_METHOD(Phalcon_Dispatcher, setReturnedValue){

	zval *value;

	phalcon_fetch_params(0, 1, 0, &value);

	phalcon_update_property_this(this_ptr, SL("_returnedValue"), value);

}

/**
 * Returns value returned by the lastest dispatched action
 *
 * @return mixed
 */
PHP_METHOD(Phalcon_Dispatcher, getReturnedValue){


	RETURN_MEMBER(this_ptr, "_returnedValue");
}

/**
 * Dispatches a handle action taking into account the routing parameters
 *
 * @return object
 */
PHP_METHOD(Phalcon_Dispatcher, dispatch){

	zval *exception_code = NULL, *exception_message = NULL;
	zval *status = NULL, *value = NULL, *handler = NULL;
	zval *camelized_class = NULL, *handler_class = NULL, *has_service = NULL;
	zval *was_fresh = NULL, *action_method = NULL, *params = NULL, *call_object = NULL;
	zval *exception = NULL, *dependency_injector, *events_manager, *event_name= NULL, *tmp;
	zval *handler_suffix, *action_suffix, *namespace_name, *handler_name, *action_name;
	zval *camelize, *camelized_namespace = NULL;
	int number_dispatches = 0;
	int has = 0, max_dispatches = 256;

	PHALCON_MM_GROW();

	dependency_injector = phalcon_read_property(this_ptr, SL("_dependencyInjector"), PH_NOISY);
	if (!dependency_injector || Z_TYPE_P(dependency_injector) != IS_OBJECT) {
		PHALCON_INIT_VAR(exception_code);
		ZVAL_LONG(exception_code, PHALCON_EXCEPTION_NO_DI);

		PHALCON_INIT_VAR(exception_message);
		ZVAL_STRING(exception_message, "A dependency injection container is required to access related dispatching services");
		PHALCON_CALL_METHOD(NULL, this_ptr, "_throwdispatchexception", exception_message, exception_code);
		RETURN_MM();
	}

	events_manager = phalcon_read_property(this_ptr, SL("_eventsManager"), PH_NOISY);
	if (events_manager && Z_TYPE_P(events_manager) != IS_OBJECT) {
		events_manager = NULL;
	}

	/**
	 * Calling beforeDispatchLoop
	 */
	PHALCON_INIT_NVAR(event_name);
	ZVAL_STRING(event_name, "dispatch:beforeDispatchLoop");
	PHALCON_CALL_METHOD(&status, this_ptr, "fireevent", event_name);
	if (PHALCON_IS_FALSE(status)) {
		RETURN_MM_FALSE;
	}

	handler_suffix = phalcon_read_property(this_ptr, SL("_handlerSuffix"), PH_NOISY);
	action_suffix  = phalcon_read_property(this_ptr, SL("_actionSuffix"), PH_NOISY);

	/**
	 * Do at least one dispatch
	 */
	phalcon_update_property_this(this_ptr, SL("_finished"), &PHALCON_GLOBAL(z_false));

	do {

		/**
		 * Loop until finished is false
		 */
		tmp = phalcon_read_property(this_ptr, SL("_finished"), PH_NOISY);
		if (zend_is_true(tmp)) {
			break;
		}

		++number_dispatches;

		/**
		 * Throw an exception after 256 consecutive forwards
		 */
		if (number_dispatches == max_dispatches) {
			PHALCON_INIT_NVAR(exception_code);
			ZVAL_LONG(exception_code, PHALCON_EXCEPTION_CYCLIC_ROUTING);

			PHALCON_INIT_NVAR(exception_message);
			ZVAL_STRING(exception_message, "Dispatcher has detected a cyclic routing causing stability problems");
			PHALCON_CALL_METHOD(NULL, this_ptr, "_throwdispatchexception", exception_message, exception_code);
			break;
		}

		phalcon_update_property_this(this_ptr, SL("_finished"), &PHALCON_GLOBAL(z_true));

		/**
		 * If the current namespace is null we used the set in this_ptr::_defaultNamespace
		 */
		namespace_name = phalcon_read_property(this_ptr, SL("_namespaceName"), PH_NOISY);
		if (!zend_is_true(namespace_name)) {
			namespace_name = phalcon_read_property(this_ptr, SL("_defaultNamespace"), PH_NOISY);
			phalcon_update_property_this(this_ptr, SL("_namespaceName"), namespace_name);
		}

		/**
		 * If the handler is null we use the set in this_ptr::_defaultHandler
		 */
		handler_name = phalcon_read_property(this_ptr, SL("_handlerName"), PH_NOISY);
		if (!zend_is_true(handler_name)) {
			handler_name = phalcon_read_property(this_ptr, SL("_defaultHandler"), PH_NOISY);
			phalcon_update_property_this(this_ptr, SL("_handlerName"), handler_name);
		}

		/**
		 * If the action is null we use the set in this_ptr::_defaultAction
		 */
		action_name = phalcon_read_property(this_ptr, SL("_actionName"), PH_NOISY);
		if (!zend_is_true(action_name)) {
			action_name = phalcon_read_property(this_ptr, SL("_defaultAction"), PH_NOISY);
			phalcon_update_property_this(this_ptr, SL("_actionName"), action_name);
		}

		/**
		 * Calling beforeDispatch
		 */
		if (events_manager) {
			PHALCON_INIT_NVAR(event_name);
			ZVAL_STRING(event_name, "dispatch:beforeDispatch");
			PHALCON_CALL_METHOD(&status, this_ptr, "fireevent", event_name);
			if (PHALCON_IS_FALSE(status)) {
				continue;
			}

			/**
			 * Check if the user made a forward in the listener
			 */
			tmp = phalcon_read_property(this_ptr, SL("_finished"), PH_NOISY);
			if (PHALCON_IS_FALSE(tmp)) {
				continue;
			}
		}

		/**
		 * We don't camelize the classes if they are in namespaces
		 */
		if (!phalcon_memnstr_str(handler_name, SL("\\"))) {
			camelize = phalcon_read_property(this_ptr, SL("_camelizeController"), PH_NOISY);
			if (!zend_is_true(camelize)) {
				PHALCON_CPY_WRT(camelized_class, handler_name);
			} else {	
				PHALCON_INIT_NVAR(camelized_class);
				phalcon_camelize(camelized_class, handler_name);
			}
		} else if (phalcon_start_with_str(handler_name, SL("\\"))) {
			PHALCON_INIT_NVAR(camelized_class);
			ZVAL_STRINGL(camelized_class, Z_STRVAL_P(handler_name)+1, Z_STRLEN_P(handler_name)-1);
		} else {
			camelized_class = handler_name;
		}

		/**
		 * Create the complete controller class name prepending the namespace
		 */
		PHALCON_INIT_NVAR(handler_class);
		if (zend_is_true(namespace_name)) {			
			camelize = phalcon_read_property(this_ptr, SL("_camelizeNamespace"), PH_NOISY);
			if (!zend_is_true(camelize)) {
				PHALCON_CPY_WRT(camelized_namespace, namespace_name);
			} else {
				PHALCON_INIT_NVAR(camelized_namespace);
				phalcon_camelize(camelized_namespace, namespace_name);
			}
			if (phalcon_end_with_str(camelized_namespace, SL("\\"))) {
				PHALCON_CONCAT_VVV(handler_class, camelized_namespace, camelized_class, handler_suffix);
			} else {
				PHALCON_CONCAT_VSVV(handler_class, camelized_namespace, "\\", camelized_class, handler_suffix);
			}
		} else {
			PHALCON_CONCAT_VV(handler_class, camelized_class, handler_suffix);
		}

		/**
		 * Handlers are retrieved as shared instances from the Service Container
		 */
		PHALCON_CALL_METHOD(&has_service, dependency_injector, "has", handler_class);
		if (!zend_is_true(has_service)) {
			/**
			 * DI doesn't have a service with that name, try to load it using an autoloader
			 */
			assert(Z_TYPE_P(handler_class) == IS_STRING);
			has = (phalcon_class_exists(Z_STR_P(handler_class), 1) != NULL) ? 1 : 0;
		} else {
			has = zend_is_true(has_service) ? 1 : 0;
		}

		/**
		 * If the service cannot be loaded we throw an exception
		 */
		if (!has) {
			PHALCON_INIT_NVAR(exception_code);
			ZVAL_LONG(exception_code, PHALCON_EXCEPTION_HANDLER_NOT_FOUND);

			PHALCON_INIT_NVAR(exception_message);
			PHALCON_CONCAT_VS(exception_message, handler_class, " handler class cannot be loaded");

			PHALCON_CALL_METHOD(&status, this_ptr, "_throwdispatchexception", exception_message, exception_code);
			if (PHALCON_IS_FALSE(status)) {

				/**
				 * Check if the user made a forward in the listener
				 */
				tmp = phalcon_read_property(this_ptr, SL("_finished"), PH_NOISY);
				if (PHALCON_IS_FALSE(tmp)) {
					continue;
				}
			}

			break;
		}

		/**
		 * Handlers must be only objects
		 */
		PHALCON_CALL_METHOD(&handler, dependency_injector, "getshared", handler_class);
		if (Z_TYPE_P(handler) != IS_OBJECT) {

			PHALCON_INIT_NVAR(exception_code);
			ZVAL_LONG(exception_code, PHALCON_EXCEPTION_INVALID_HANDLER);

			PHALCON_INIT_NVAR(exception_message);
			ZVAL_STRING(exception_message, "Invalid handler returned from the services container");

			PHALCON_CALL_METHOD(&status, this_ptr, "_throwdispatchexception", exception_message, exception_code);
			if (PHALCON_IS_FALSE(status)) {

				tmp = phalcon_read_property(this_ptr, SL("_finished"), PH_NOISY);
				if (PHALCON_IS_FALSE(tmp)) {
					continue;
				}
			}

			break;
		}

		/**
		 * If the object was recently created in the DI we initialize it
		 */
		PHALCON_CALL_METHOD(&was_fresh, dependency_injector, "wasfreshinstance");

		/**
		 * Update the active handler making it available for events
		 */
		phalcon_update_property_this(this_ptr, SL("_activeHandler"), handler);

		/**
		 * Check if the method exists in the handler
		 */
		PHALCON_INIT_NVAR(action_method);
		PHALCON_CONCAT_VV(action_method, action_name, action_suffix);
		if (phalcon_method_exists(handler, action_method) == FAILURE) {

			/**
			 * Call beforeNotFoundAction
			 */
			if (events_manager) {
				PHALCON_INIT_NVAR(event_name);
				ZVAL_STRING(event_name, "dispatch:beforeNotFoundAction");
				PHALCON_CALL_METHOD(&status, this_ptr, "fireevent", event_name);
				if (PHALCON_IS_FALSE(status)) {
					continue;
				}

				tmp = phalcon_read_property(this_ptr, SL("_finished"), PH_NOISY);
				if (PHALCON_IS_FALSE(tmp)) {
					continue;
				}
			}

			PHALCON_INIT_NVAR(exception_code);
			ZVAL_LONG(exception_code, PHALCON_EXCEPTION_ACTION_NOT_FOUND);

			PHALCON_INIT_NVAR(exception_message);
			PHALCON_CONCAT_SVSVS(exception_message, "Action '", action_name, "' was not found on handler '", handler_name, "'");

			/**
			 * Try to throw an exception when an action isn't defined on the object
			 */
			PHALCON_CALL_METHOD(&status, this_ptr, "_throwdispatchexception", exception_message, exception_code);
			if (PHALCON_IS_FALSE(status)) {

				tmp = phalcon_read_property(this_ptr, SL("_finished"), PH_NOISY);
				if (PHALCON_IS_FALSE(tmp)) {
					continue;
				}
			}

			break;
		}

		/**
		 * Calling beforeExecuteRoute
		 */
		if (events_manager) {
			PHALCON_INIT_NVAR(event_name);
			ZVAL_STRING(event_name, "dispatch:beforeExecuteRoute");
			PHALCON_CALL_METHOD(&status, this_ptr, "fireevent", event_name);
			if (PHALCON_IS_FALSE(status)) {
				continue;
			}

			/**
			 * Check if the user made a forward in the listener
			 */
			tmp = phalcon_read_property(this_ptr, SL("_finished"), PH_NOISY);
			if (PHALCON_IS_FALSE(tmp)) {
				continue;
			}
		}

		/**
		 * Calling beforeExecuteRoute as callback and event
		 */
		if (phalcon_method_exists_ex(handler, SS("beforeexecuteroute")) == SUCCESS) {
			PHALCON_CALL_METHOD(&status, handler, "beforeexecuteroute", this_ptr);
			if (PHALCON_IS_FALSE(status)) {
				continue;
			}

			/**
			 * Check if the user made a forward in the listener
			 */
			tmp = phalcon_read_property(this_ptr, SL("_finished"), PH_NOISY);
			if (PHALCON_IS_FALSE(tmp)) {
				continue;
			}
		}

		/**
		 * Call the 'initialize' method just once per request
		 */
		if (PHALCON_IS_TRUE(was_fresh)) {
			if (phalcon_method_exists_ex(handler, SS("initialize")) == SUCCESS) {
				PHALCON_CALL_METHOD(NULL, handler, "initialize");
			}

			/**
			 * Calling afterInitialize
			 */
			if (events_manager) {
				PHALCON_INIT_NVAR(event_name);
				ZVAL_STRING(event_name, "dispatch:afterInitialize");
				PHALCON_CALL_METHOD(&status, this_ptr, "fireevent", event_name);
				if (PHALCON_IS_FALSE(status)) {
					continue;
				}

				/**
				 * Check if the user made a forward in the listener
				 */
				tmp = phalcon_read_property(this_ptr, SL("_finished"), PH_NOISY);
				if (PHALCON_IS_FALSE(tmp)) {
					continue;
				}
			}
		}

		/**
		 * Check if the params is an array
		 */
		params = phalcon_read_property(this_ptr, SL("_params"), PH_NOISY);
		if (Z_TYPE_P(params) != IS_ARRAY) {

			PHALCON_INIT_NVAR(exception_code);
			ZVAL_LONG(exception_code, PHALCON_EXCEPTION_INVALID_PARAMS);

			PHALCON_INIT_NVAR(exception_message);
			ZVAL_STRING(exception_message, "Action parameters must be an Array");

			/**
			 * An invalid parameter variable was passed throw an exception
			 */
			PHALCON_CALL_METHOD(&status, this_ptr, "_throwdispatchexception", exception_message, exception_code);
			if (PHALCON_IS_FALSE(status)) {

				tmp = phalcon_read_property(this_ptr, SL("_finished"), PH_NOISY);
				if (PHALCON_IS_FALSE(tmp)) {
					continue;
				}
			}

			break;
		}

		/**
		 * Create a call handler
		 */
		PHALCON_INIT_NVAR(call_object);
		array_init_size(call_object, 2);
		phalcon_array_append(&call_object, handler, 0);
		phalcon_array_append(&call_object, action_method, 0);

		PHALCON_INIT_NVAR(value);
		/* Call the method allowing exceptions */
		PHALCON_CALL_USER_FUNC_ARRAY_NOEX(value, call_object, params);

		/* Check if an exception has ocurred */
		if (EG(exception)) {
			zval e;
			ZVAL_OBJ(&e, EG(exception));

			/* Copy the exception to rethrow it later if needed */
			PHALCON_CPY_WRT(exception, &e);

			/* Clear the exception  */
			zend_clear_exception();

			/* Try to handle the exception */
			PHALCON_CALL_METHOD(&status, this_ptr, "_handleexception", exception);
			if (PHALCON_IS_FALSE(status)) {
				tmp = phalcon_read_property(this_ptr, SL("_finished"), PH_NOISY);
				if (PHALCON_IS_FALSE(tmp)) {
					continue;
				}
			}
			else {
				/* Exception was not handled, rethrow it */
				phalcon_throw_exception(exception);
				RETURN_MM();
			}
		} else {
			/* Update the latest value produced by the latest handler */
			phalcon_update_property_this(this_ptr, SL("_returnedValue"), value);
		}

		phalcon_update_property_this(this_ptr, SL("_lastHandler"), handler);

		if (events_manager) {
			/**
			 * Call afterExecuteRoute
			 */
			PHALCON_INIT_NVAR(event_name);
			ZVAL_STRING(event_name, "dispatch:afterExecuteRoute");
			PHALCON_CALL_METHOD(&status, this_ptr, "fireevent", event_name);
			if (PHALCON_IS_FALSE(status)) {
				continue;
			}

			tmp = phalcon_read_property(this_ptr, SL("_finished"), PH_NOISY);
			if (PHALCON_IS_FALSE(tmp)) {
				continue;
			}

			/**
			 * Call afterDispatch
			 */
			PHALCON_INIT_NVAR(event_name);
			ZVAL_STRING(event_name, "dispatch:afterDispatch");
			PHALCON_CALL_METHOD(&status, this_ptr, "fireevent", event_name);
		}

		/**
		 * Calling afterExecuteRoute as callback and event
		 */
		if (phalcon_method_exists_ex(handler, SS("afterexecuteroute")) == SUCCESS) {
			PHALCON_CALL_METHOD(&status, handler, "afterexecuteroute", this_ptr, value);
			if (PHALCON_IS_FALSE(status)) {
				continue;
			}

			tmp = phalcon_read_property(this_ptr, SL("_finished"), PH_NOISY);
			if (PHALCON_IS_FALSE(tmp)) {
				continue;
			}
		}
	} while (number_dispatches <= max_dispatches);

	/**
	 * Call afterDispatchLoop
	 */
	PHALCON_INIT_NVAR(event_name);
	ZVAL_STRING(event_name, "dispatch:afterDispatchLoop");
	PHALCON_CALL_METHOD(NULL, this_ptr, "fireevent", event_name);

	if (handler) {
		RETURN_CCTOR(handler);
	}

	RETURN_MM_NULL();
}

/**
 * Forwards the execution flow to another controller/action
 * Dispatchers are unique per module. Forwarding between modules is not allowed
 *
 *<code>
 *  $this->dispatcher->forward(array('controller' => 'posts', 'action' => 'index'));
 *</code>
 *
 * @param string|array $forward
 * @return bool
 */
PHP_METHOD(Phalcon_Dispatcher, forward){

	zval *forward, *forward_parts = NULL, *parts, *number_parts, *controller_part;
	zval *real_namespace_name, *real_controller_name = NULL, *action_part, *exception_code, *exception_message;
	zval *namespace_name, *controller_name, *task_name, *action_name, *params;
	zval *previous_namespace_name, *previous_controller_name, *previous_action_name, *previous_params;
	int num = 0;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 1, 0, &forward);

	if (Z_TYPE_P(forward) == IS_STRING) {
		PHALCON_INIT_VAR(forward_parts);
		array_init(forward_parts);

		PHALCON_INIT_VAR(parts);
		phalcon_fast_explode_str(parts, SL("::"), forward);

		PHALCON_INIT_VAR(number_parts);
		phalcon_fast_count(number_parts, parts);

		num = phalcon_get_intval(number_parts);

		if (num > 0 && num <= 3) {
			switch (num) {

				case 3:
					PHALCON_OBS_VAR(controller_part);
					phalcon_array_fetch_long(&controller_part, parts, 1, PH_NOISY);

					PHALCON_OBS_VAR(action_part);
					phalcon_array_fetch_long(&action_part, parts, 2, PH_NOISY);
					break;

				case 2:
					PHALCON_OBS_VAR(controller_part);
					phalcon_array_fetch_long(&controller_part, parts, 0, PH_NOISY);

					PHALCON_OBS_VAR(action_part);
					phalcon_array_fetch_long(&action_part, parts, 1, PH_NOISY);

					break;

				case 1:
					PHALCON_OBS_VAR(controller_part);
					phalcon_array_fetch_long(&controller_part, parts, 0, PH_NOISY);

					PHALCON_INIT_VAR(action_part);
					break;

			}

			if (phalcon_memnstr_str(controller_part, SL("\\"))) {
				PHALCON_INIT_VAR(real_controller_name);
				phalcon_get_class_ns(real_controller_name, controller_part, 0);

				phalcon_array_update_string(&forward_parts, SL("controller"), real_controller_name, PH_COPY);

				PHALCON_INIT_VAR(real_namespace_name);
				phalcon_get_ns_class(real_namespace_name, controller_name, 0);

				if (zend_is_true(namespace_name)) {
					phalcon_array_update_string(&forward_parts, SL("namespace"), real_namespace_name, PH_COPY);
				}
			} else {
				PHALCON_CPY_WRT(real_controller_name, controller_part);
			}

			PHALCON_INIT_NVAR(controller_part);
			phalcon_uncamelize(controller_part, real_controller_name);
			phalcon_array_update_string(&forward_parts, SL("controller"), controller_part, PH_COPY);

			if (Z_TYPE_P(action_part) != IS_NULL) {
				phalcon_array_update_string(&forward_parts, SL("action"), action_part, PH_COPY);
			}
		}
	} else {
		PHALCON_CPY_WRT(forward_parts, forward);
	}

	if (Z_TYPE_P(forward_parts) != IS_ARRAY) {
		PHALCON_INIT_VAR(exception_code);
		ZVAL_LONG(exception_code, PHALCON_EXCEPTION_INVALID_PARAMS);

		PHALCON_INIT_VAR(exception_message);
		ZVAL_STRING(exception_message, "Forward parameter must be an Array");
		PHALCON_CALL_METHOD(NULL, this_ptr, "_throwdispatchexception", exception_message);
		RETURN_MM_NULL();
	}

	previous_namespace_name = phalcon_read_property(this_ptr, SL("_namespaceName"), PH_NOISY);
	phalcon_update_property_this(this_ptr, SL("_previousNamespaceName"), previous_namespace_name);

	previous_controller_name = phalcon_read_property(this_ptr, SL("_handlerName"), PH_NOISY);
	phalcon_update_property_this(this_ptr, SL("_previousHandlerName"), previous_controller_name);

	previous_action_name = phalcon_read_property(this_ptr, SL("_actionName"), PH_NOISY);
	phalcon_update_property_this(this_ptr, SL("_previousActionName"), previous_action_name);

	previous_params = phalcon_read_property(this_ptr, SL("_params"), PH_NOISY);
	phalcon_update_property_this(this_ptr, SL("_previousParams"), previous_params);

	/**
	 * Check if we need to forward to another namespace
	 */
	if (phalcon_array_isset_string_fetch(&namespace_name, forward_parts, SS("namespace"))) {
		phalcon_update_property_this(this_ptr, SL("_namespaceName"), namespace_name);
	}

	/**
	 * Check if we need to forward to another controller
	 */
	if (phalcon_array_isset_string_fetch(&controller_name, forward_parts, SS("controller"))) {
		phalcon_update_property_this(this_ptr, SL("_handlerName"), controller_name);
	} else {
		if (phalcon_array_isset_string_fetch(&task_name, forward_parts, SS("task"))) {
			phalcon_update_property_this(this_ptr, SL("_handlerName"), task_name);
		}
	}

	/**
	 * Check if we need to forward to another action
	 */
	if (phalcon_array_isset_string_fetch(&action_name, forward_parts, SS("action"))) {
		phalcon_update_property_this(this_ptr, SL("_actionName"), action_name);
	}

	/**
	 * Check if we need to forward changing the current parameters
	 */
	if (phalcon_array_isset_string_fetch(&params, forward_parts, SS("params"))) {
		phalcon_update_property_this(this_ptr, SL("_params"), params);
	}

	phalcon_update_property_this(this_ptr, SL("_finished"), &PHALCON_GLOBAL(z_false));
	phalcon_update_property_this(this_ptr, SL("_forwarded"), &PHALCON_GLOBAL(z_true));

	RETURN_MM();
}

/**
 * Check if the current executed action was forwarded by another one
 *
 * @return boolean
 */
PHP_METHOD(Phalcon_Dispatcher, wasForwarded){


	RETURN_MEMBER(this_ptr, "_forwarded");
}

/**
 * Possible class name that will be located to dispatch the request
 *
 * @return string
 */
PHP_METHOD(Phalcon_Dispatcher, getHandlerClass){

	zval *camelized_class = NULL;
	zval *handler_suffix, *namespace_name, *handler_name, *camelize;
	zval *camelized_namespace = NULL;

	PHALCON_MM_GROW();

	/**
	 * The handler suffix
	 */
	handler_suffix = phalcon_read_property(this_ptr, SL("_handlerSuffix"), PH_NOISY);

	/**
	 * If the current namespace is null we used the set in this_ptr::_defaultNamespace
	 */
	namespace_name = phalcon_read_property(this_ptr, SL("_namespaceName"), PH_NOISY);
	if (!zend_is_true(namespace_name)) {
		namespace_name = phalcon_read_property(this_ptr, SL("_defaultNamespace"), PH_NOISY);
		phalcon_update_property_this(this_ptr, SL("_namespaceName"), namespace_name);
	}

	/**
	 * If the handler is null we use the set in this_ptr::_defaultHandler
	 */
	handler_name = phalcon_read_property(this_ptr, SL("_handlerName"), PH_NOISY);
	if (!zend_is_true(handler_name)) {
		handler_name = phalcon_read_property(this_ptr, SL("_defaultHandler"), PH_NOISY);
		phalcon_update_property_this(this_ptr, SL("_handlerName"), handler_name);
	}

	/**
	 * We don't camelize the classes if they are in namespaces
	 */
	if (!phalcon_memnstr_str(handler_name, SL("\\"))) {
		PHALCON_INIT_VAR(camelized_class);
		phalcon_camelize(camelized_class, handler_name);
	} else if (phalcon_start_with_str(handler_name, SL("\\"))) {
		PHALCON_INIT_VAR(camelized_class);
		ZVAL_STRINGL(camelized_class, Z_STRVAL_P(handler_name)+1, Z_STRLEN_P(handler_name)-1);
	} else {
		camelized_class = handler_name;
	}

	/**
	 * Create the complete controller class name prepending the namespace
	 */
	if (zend_is_true(namespace_name)) {
		camelize = phalcon_read_property(this_ptr, SL("_camelizeNamespace"), PH_NOISY);
		if (!zend_is_true(camelize)) {
			PHALCON_CPY_WRT(camelized_namespace, namespace_name);
		} else {
			PHALCON_INIT_NVAR(camelized_namespace);
			phalcon_camelize(camelized_namespace, namespace_name);
		}
		if (phalcon_end_with_str(camelized_namespace, SL("\\"))) {
			PHALCON_CONCAT_VVV(return_value, camelized_namespace, camelized_class, handler_suffix);
		} else {
			PHALCON_CONCAT_VSVV(return_value, camelized_namespace, "\\", camelized_class, handler_suffix);
		}
	} else {
		PHALCON_CONCAT_VV(return_value, camelized_class, handler_suffix);
	}

	phalcon_update_property_this(this_ptr, SL("_isExactHandler"), &PHALCON_GLOBAL(z_false));

	PHALCON_MM_RESTORE();
}

/**
 * Enables/Disables automatically camelize namespace 
 *
 *<code>
 *  $this->dispatcher->camelizeNamespace(FALSE);
 *</code>
 *
 * @param bool $camelize
 */
PHP_METHOD(Phalcon_Dispatcher, camelizeNamespace){

	zval *camelize;

	phalcon_fetch_params(0, 1, 0, &camelize);

	if (PHALCON_IS_TRUE(camelize)) {
		phalcon_update_property_this(this_ptr, SL("_camelizeNamespace"), &PHALCON_GLOBAL(z_true));
	} else {
		phalcon_update_property_this(this_ptr, SL("_camelizeNamespace"), &PHALCON_GLOBAL(z_false));
	}
}

/**
 * Enables/Disables automatically camelize controller 
 *
 *<code>
 *  $this->dispatcher->camelizeController(FALSE);
 *</code>
 *
 * @param bool $camelize
 */
PHP_METHOD(Phalcon_Dispatcher, camelizeController){

	zval *camelize;

	phalcon_fetch_params(0, 1, 0, &camelize);

	if (PHALCON_IS_TRUE(camelize)) {
		phalcon_update_property_this(this_ptr, SL("_camelizeController"), &PHALCON_GLOBAL(z_true));
	} else {
		phalcon_update_property_this(this_ptr, SL("_camelizeController"), &PHALCON_GLOBAL(z_false));
	}
}

/**
 * Set error handler
 *
 * @param mixed $handler
 * @param int $exception_code
 * @return Phalcon\DispatcherInterface
 */
PHP_METHOD(Phalcon_Dispatcher, setErrorHandler){

	zval *error_handler, *exception_code = NULL;

	phalcon_fetch_params(0, 1, 1, &error_handler, &exception_code);

	if (!exception_code) {
		exception_code = &PHALCON_GLOBAL(z_zero);
	}

	phalcon_update_property_array(this_ptr, SL("_errorHandlers"), exception_code, error_handler);

	RETURN_THISW();
}

/**
 * Get error handler
 *
 * @param int $exception_code
 * @return mixed
 */
PHP_METHOD(Phalcon_Dispatcher, getErrorHandler){

	zval *exception_code = NULL, *error_handlers, *error_handler = NULL;

	phalcon_fetch_params(0, 0, 1, &exception_code);

	if (!exception_code) {
		exception_code = &PHALCON_GLOBAL(z_zero);
	}

	error_handlers = phalcon_read_property(this_ptr, SL("_errorHandlers"), PH_NOISY);

	if (Z_TYPE_P(error_handlers) == IS_ARRAY) {
		if (phalcon_array_isset_fetch(&error_handler, error_handlers, exception_code)) {
			RETURN_CCTORW(error_handler);
		}
	}

	RETURN_NULL();
}

/**
 * Fires an event, implicitly calls behaviors and listeners in the events manager are notified
 *
 * @param string $eventName
 * @param string $data
 * @param string $cancelable
 * @return boolean
 */
PHP_METHOD(Phalcon_Dispatcher, fireEvent){

	zval *event_name, *data = NULL, *cancelable = NULL, status;
	int ret, ret2;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 1, 2, &event_name, &data, &cancelable);
	PHALCON_SEPARATE_PARAM(event_name);

	if (!data) {
		data = &PHALCON_GLOBAL(z_null);
	}

	if (!cancelable) {
		cancelable = &PHALCON_GLOBAL(z_null);
	}

	Z_SET_ISREF_P(data);
	zval *params[] = {PHALCON_FETCH_VA_ARGS event_name, data, cancelable};
	ret = phalcon_call_class_method_aparams(&status, getThis(), phalcon_dispatcher_ce, phalcon_fcall_parent, SL("fireevent"), PHALCON_CALL_NUM_PARAMS(params), PHALCON_PASS_CALL_PARAMS(params));
	Z_UNSET_ISREF_P(data);

	if (EG(exception)) {
		zval exception;
		ZVAL_OBJ(&exception, EG(exception));

		zend_clear_exception();

		/* Shortcut, save one method call */
		PHALCON_INIT_NVAR(event_name);
		ZVAL_STRING(event_name, "dispatch:beforeException");

		zval *params[] = {PHALCON_FETCH_VA_ARGS event_name, exception};
		ret2 = phalcon_call_class_method_aparams(&status, getThis(), phalcon_dispatcher_ce, phalcon_fcall_parent, SL("fireevent"), PHALCON_CALL_NUM_PARAMS(params), PHALCON_PASS_CALL_PARAMS(params));

		if (ret2 == SUCCESS && PHALCON_IS_FALSE(&status)) {
			RETURN_MM_FALSE;
		}
	}

	if (ret == FAILURE || PHALCON_IS_FALSE(&status)) {
		RETURN_MM_FALSE;
	}

	RETURN_MM_TRUE;
}
