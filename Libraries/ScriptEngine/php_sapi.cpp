/* Copyright 2013-2015 Sathya Laufer
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Homegear.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

#include "php_sapi.h"
#include "../GD/GD.h"
#include "PHPVariableConverter.h"

#define SEG(v) TSRMG(homegear_globals_id, zend_homegear_globals*, v)

static int homegear_globals_id;

zend_homegear_globals* php_homegear_get_globals(TSRMLS_D)
{
	return ((zend_homegear_globals*) (*((void ***) tsrm_ls))[((homegear_globals_id)-1)]);
}

void php_homegear_build_argv(std::vector<std::string>& arguments TSRMLS_DC)
{
	zval* argc = nullptr;
	zval* argv = nullptr;
	ALLOC_INIT_ZVAL(argv);
	array_init(argv);
	zval* arg = nullptr;

	for(std::vector<std::string>::const_iterator i = arguments.begin(); i != arguments.end(); ++i)
	{
		ALLOC_ZVAL(arg);
		Z_TYPE_P(arg) = IS_STRING;
		Z_STRLEN_P(arg) = (*i).size();
		Z_STRVAL_P(arg) = estrndup((*i).c_str(), Z_STRLEN_P(arg));
		INIT_PZVAL(arg);
		if (zend_hash_next_index_insert(Z_ARRVAL_P(argv), &arg, sizeof(zval*), NULL) == FAILURE) {
			if (Z_TYPE_P(arg) == IS_STRING) {
				efree(Z_STRVAL_P(arg));
			}
		}
	}

	ALLOC_INIT_ZVAL(argc);
	Z_LVAL_P(argc) = arguments.size();
	Z_TYPE_P(argc) = IS_LONG;

	Z_ADDREF_P(argv);
	Z_ADDREF_P(argc);

	zend_hash_update(&EG(symbol_table), "argv", sizeof("argv"), &argv, sizeof(zval *), NULL);
	zend_hash_update(&EG(symbol_table), "argc", sizeof("argc"), &argc, sizeof(zval *), NULL);

	zval_ptr_dtor(&argc);
	zval_ptr_dtor(&argv);
}

static int php_homegear_read_post(char *buf, uint count_bytes TSRMLS_DC)
{
	std::cout << "php_homegear_read_post" << std::endl;
	return count_bytes;
}

static char* php_homegear_read_cookies(TSRMLS_D)
{
	std::cout << "php_homegear_read_cookies" << std::endl;
	return NULL;
}

static int php_homegear_ub_write(const char* str, uint32_t length TSRMLS_DC)
{
	if(length == 0) return 0;
	std::vector<char>* out = SEG(output);
	if(out)
	{
		if(out->size() + length > out->capacity()) out->reserve(out->capacity() + 1024);
		out->insert(out->end(), str, str + length);
	}
	else GD::out.printMessage("Script output: " + std::string(str, length));
	return length;
}

static void php_homegear_flush(void *server_context)
{
	//We are normally buffering only, so no flush is needed.
}

static void php_homegear_send_header(sapi_header_struct *sapi_header, void *server_context TSRMLS_DC)
{
	if(!sapi_header) return;
	std::string header(sapi_header->header, sapi_header->header_len);
	std::cout << "php_homegear_send_header: " << header << std::endl;
}

static void php_homegear_log_message(char *message TSRMLS_DC)
{
	fprintf (stderr, "%s\n", message);
}

static void php_homegear_register_variables(zval *track_vars_array TSRMLS_DC)
{
	php_register_variable((char*)"SAPI_TYPE", (char*)"Homegear", track_vars_array TSRMLS_CC);

	php_import_environment_variables(track_vars_array TSRMLS_CC);
}

static int php_homegear_startup(sapi_module_struct *sapi_module)
{
	if (php_module_startup(sapi_module, NULL, 0)==FAILURE) {
		return FAILURE;
	}

	return SUCCESS;
}

static int php_homegear_activate(TSRMLS_D)
{
	return SUCCESS;
}

static int php_homegear_deactivate(TSRMLS_D)
{
	return SUCCESS;
}

sapi_module_struct php_homegear_module = {
	(char*)"homegear",                       /* name */
	(char*)"PHP Homegear Library",        /* pretty name */

	php_homegear_startup,              /* startup == MINIT. Called for each new thread. */
	php_module_shutdown_wrapper,   /* shutdown == MSHUTDOWN. Called for each new thread. */

	php_homegear_activate,	            /* activate == RINIT. Called for each request. */
	php_homegear_deactivate,  			/* deactivate == RSHUTDOWN. Called for each request. */

	php_homegear_ub_write,             /* unbuffered write */
	php_homegear_flush,                /* flush */
	NULL,                          /* get uid */
	NULL,                          /* getenv */

	php_error,                     /* error handler */

	NULL,                          /* header handler */
	NULL,                          /* send headers handler */
	php_homegear_send_header,          /* send header handler */

	php_homegear_read_post,            /* read POST data */
	php_homegear_read_cookies,         /* read Cookies */

	php_homegear_register_variables,   /* register server variables */
	php_homegear_log_message,          /* Log message */
	NULL,							/* Get request time */
	NULL,							/* Child terminate */

	STANDARD_SAPI_MODULE_PROPERTIES
};
/* }}} */

static void php_homegear_globals_ctor(zend_homegear_globals* homegear_globals TSRMLS_DC)
{

}

static void php_homegear_globals_dtor(zend_homegear_globals* homegear_globals TSRMLS_DC)
{

}

void php_homegear_invoke_rpc(std::string& methodName, std::shared_ptr<BaseLib::RPC::Variable>& parameters, int ht, zval *return_value, zval **return_value_ptr, zval *this_ptr, int return_value_used , void ***tsrm_ls)
{
	std::shared_ptr<BaseLib::RPC::Variable> result = GD::rpcServers.begin()->second.callMethod(methodName, parameters);
	if(result->errorStruct)
	{
		std::string errorString("RPC error (Code " + std::to_string(result->structValue->at("faultCode")->integerValue) + "): " + result->structValue->at("faultString")->stringValue);
		zend_error(E_ERROR, "%s", errorString.c_str());
		RETURN_FALSE;
	}
	PHPVariableConverter::getPHPVariable(result, return_value);
}

/* Script engine specific functions */

ZEND_FUNCTION(hg_auth)
{
	char* pName = nullptr;
	int32_t nameLength = 0;
	char* pPassword = nullptr;
	int32_t passwordLength = 0;
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &pName, &nameLength, &pPassword, &passwordLength) != SUCCESS) RETURN_NULL();
	if(nameLength == 0 || passwordLength == 0) RETURN_FALSE;
	if(User::verify(std::string(pName, nameLength), std::string(pPassword, passwordLength))) RETURN_TRUE;
	RETURN_FALSE;
}

ZEND_FUNCTION(hg_create_user)
{
	char* pName = nullptr;
	int32_t nameLength = 0;
	char* pPassword = nullptr;
	int32_t passwordLength = 0;
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &pName, &nameLength, &pPassword, &passwordLength) != SUCCESS) RETURN_NULL();
	if(nameLength == 0 || passwordLength == 0) RETURN_FALSE;
	if(User::create(std::string(pName, nameLength), std::string(pPassword, passwordLength))) RETURN_TRUE;
	RETURN_FALSE;
}

ZEND_FUNCTION(hg_delete_user)
{
	char* pName = nullptr;
	int32_t nameLength = 0;
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &pName, &nameLength) != SUCCESS) RETURN_NULL();
	if(nameLength == 0) RETURN_FALSE;
	if(User::remove(std::string(pName, nameLength))) RETURN_TRUE;
	RETURN_FALSE;
}

ZEND_FUNCTION(hg_invoke)
{
	int32_t argc = ZEND_NUM_ARGS();
	zval*** args = (zval ***)safe_emalloc(argc, sizeof(zval**), 0);
	if (argc == 0 || zend_get_parameters_array_ex(argc, args) == FAILURE)
	{
        efree(args);
        WRONG_PARAM_COUNT;
    }
	if(Z_TYPE_PP(args[0]) != IS_STRING)
	{
		efree(args);
		zend_error(E_ERROR, "Function requires string as first argument.");
		RETURN_FALSE;
	}
	std::string methodName(Z_STRVAL_P(*args[0]), Z_STRLEN_P(*args[0]));
	std::shared_ptr<BaseLib::RPC::Variable> parameters(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
	for(int32_t i = 1; i < argc; i++)
	{
		std::shared_ptr<BaseLib::RPC::Variable> parameter = PHPVariableConverter::getVariable(*args[i]);
		if(parameter) parameters->arrayValue->push_back(parameter);
	}
	efree(args);
	php_homegear_invoke_rpc(methodName, parameters, ht, return_value, return_value_ptr, this_ptr, return_value_used, tsrm_ls);
}

ZEND_FUNCTION(hg_update_user)
{
	char* pName = nullptr;
	int32_t nameLength = 0;
	char* pPassword = nullptr;
	int32_t passwordLength = 0;
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &pName, &nameLength, &pPassword, &passwordLength) != SUCCESS) RETURN_NULL();
	if(nameLength == 0 || passwordLength == 0) RETURN_FALSE;
	if(User::update(std::string(pName, nameLength), std::string(pPassword, passwordLength))) RETURN_TRUE;
	RETURN_FALSE;
}

ZEND_FUNCTION(hg_user_exists)
{
	char* pName = nullptr;
	int32_t nameLength = 0;
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &pName, &nameLength) != SUCCESS) RETURN_NULL();
	if(nameLength == 0) RETURN_FALSE;
	if(User::exists(std::string(pName, nameLength))) RETURN_TRUE;
	RETURN_FALSE;
}

ZEND_FUNCTION(hg_users)
{
	std::map<uint64_t, std::string> users;
	User::getAll(users);
	array_init(return_value);
	for(std::map<uint64_t, std::string>::iterator i = users.begin(); i != users.end(); ++i)
	{
		add_next_index_string(return_value, i->second.c_str(), i->second.size());
	}
}

/* RPC functions */

ZEND_FUNCTION(hg_get_meta)
{
	uint32_t id = 0;
	char* pName = nullptr;
	int32_t nameLength = 0;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ls", &id, &pName, &nameLength) != SUCCESS) RETURN_NULL();
	std::string methodName("getMetadata");
	std::shared_ptr<BaseLib::RPC::Variable> parameters(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
	parameters->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(id)));
	parameters->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(std::string(pName, nameLength))));
	php_homegear_invoke_rpc(methodName, parameters, ht, return_value, return_value_ptr, this_ptr, return_value_used, tsrm_ls);
}

ZEND_FUNCTION(hg_get_system)
{
	char* pName = nullptr;
	int32_t nameLength = 0;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",&pName, &nameLength) != SUCCESS) RETURN_NULL();
	std::string methodName("getSystemVariable");
	std::shared_ptr<BaseLib::RPC::Variable> parameters(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
	parameters->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(std::string(pName, nameLength))));
	php_homegear_invoke_rpc(methodName, parameters, ht, return_value, return_value_ptr, this_ptr, return_value_used, tsrm_ls);
}

ZEND_FUNCTION(hg_get_value)
{
	uint32_t id = 0;
	int32_t channel = -1;
	char* pParameterName = nullptr;
	int32_t parameterNameLength = 0;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lls", &id, &channel, &pParameterName, &parameterNameLength) != SUCCESS) RETURN_NULL();
	std::string methodName("getValue");
	std::shared_ptr<BaseLib::RPC::Variable> parameters(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
	parameters->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(id)));
	parameters->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(channel)));
	parameters->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(std::string(pParameterName, parameterNameLength))));
	php_homegear_invoke_rpc(methodName, parameters, ht, return_value, return_value_ptr, this_ptr, return_value_used, tsrm_ls);
}

ZEND_FUNCTION(hg_set_meta)
{
	uint32_t id = 0;
	char* pName = nullptr;
	int32_t nameLength = 0;
	zval* newValue;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lsz", &id, &pName, &nameLength, &newValue) != SUCCESS) RETURN_NULL();
	std::string methodName("setMetadata");
	std::shared_ptr<BaseLib::RPC::Variable> parameters(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
	parameters->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(id)));
	parameters->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(std::string(pName, nameLength))));
	std::shared_ptr<BaseLib::RPC::Variable> parameter = PHPVariableConverter::getVariable(newValue);
	if(parameter) parameters->arrayValue->push_back(parameter);
	php_homegear_invoke_rpc(methodName, parameters, ht, return_value, return_value_ptr, this_ptr, return_value_used, tsrm_ls);
}

ZEND_FUNCTION(hg_set_system)
{
	char* pName = nullptr;
	int32_t nameLength = 0;
	zval* newValue;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &pName, &nameLength, &newValue) != SUCCESS) RETURN_NULL();
	std::string methodName("setSystemVariable");
	std::shared_ptr<BaseLib::RPC::Variable> parameters(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
	parameters->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(std::string(pName, nameLength))));
	std::shared_ptr<BaseLib::RPC::Variable> parameter = PHPVariableConverter::getVariable(newValue);
	if(parameter) parameters->arrayValue->push_back(parameter);
	php_homegear_invoke_rpc(methodName, parameters, ht, return_value, return_value_ptr, this_ptr, return_value_used, tsrm_ls);
}

ZEND_FUNCTION(hg_set_value)
{
	uint32_t id = 0;
	int32_t channel = -1;
	char* pParameterName = nullptr;
	int32_t parameterNameLength = 0;
	zval* newValue;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "llsz", &id, &channel, &pParameterName, &parameterNameLength, &newValue) != SUCCESS) RETURN_NULL();
	std::string methodName("setValue");
	std::shared_ptr<BaseLib::RPC::Variable> parameters(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
	parameters->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(id)));
	parameters->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(channel)));
	parameters->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(std::string(pParameterName, parameterNameLength))));
	std::shared_ptr<BaseLib::RPC::Variable> parameter = PHPVariableConverter::getVariable(newValue);
	if(parameter) parameters->arrayValue->push_back(parameter);
	php_homegear_invoke_rpc(methodName, parameters, ht, return_value, return_value_ptr, this_ptr, return_value_used, tsrm_ls);
}

static const zend_function_entry additional_functions[] = {
	ZEND_FE(hg_auth, NULL)
	ZEND_FE(hg_create_user, NULL)
	ZEND_FE(hg_delete_user, NULL)
	ZEND_FE(hg_invoke, NULL)
	ZEND_FE(hg_update_user, NULL)
	ZEND_FE(hg_user_exists, NULL)
	ZEND_FE(hg_users, NULL)
	ZEND_FE(hg_get_meta, NULL)
	ZEND_FE(hg_get_system, NULL)
	ZEND_FE(hg_get_value, NULL)
	ZEND_FE(hg_set_meta, NULL)
	ZEND_FE(hg_set_system, NULL)
	ZEND_FE(hg_set_value, NULL)
	{NULL, NULL, NULL}
};

int php_homegear_init()
{
	tsrm_startup(1, 1, 0, NULL);
	sapi_startup(&php_homegear_module);
	php_homegear_module.additional_functions = additional_functions;
	sapi_module.startup(&php_homegear_module);

	ts_allocate_id(&homegear_globals_id, sizeof(zend_homegear_globals), (void(*)(void*, void***))php_homegear_globals_ctor, (void(*)(void*, void***))php_homegear_globals_dtor);

	return SUCCESS;
}

void php_homegear_shutdown()
{
	php_homegear_module.shutdown(&php_homegear_module);
	sapi_shutdown();
	tsrm_shutdown();
}