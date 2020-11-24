/*
 * Wazuh Module for Task management.
 * Copyright (C) 2015-2020, Wazuh Inc.
 * July 13, 2020.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#ifdef WAZUH_UNIT_TESTING
// Remove static qualifier when unit testing
#define STATIC
#else
#define STATIC static
#endif

#ifndef WIN32

#include "../wmodules.h"
#include "wm_task_manager_parsing.h"
#include "defs.h"
#include "wazuhdb_op.h"

#define WDBQUERY_SIZE OS_BUFFER_SIZE
#define WDBOUTPUT_SIZE OS_MAXSTR

/**
 * Analyze an upgrade or upgrade_custom command. Update the tasks DB when necessary.
 * @param task Upgrade task to be processed.
 * @param command Command of the task to be executed.
 * @param error_code Variable to store an error code if something is wrong.
 * @return JSON object with the response for this task.
 * */
STATIC cJSON* wm_task_manager_command_upgrade(wm_task_manager_upgrade *task, int command, int *error_code) __attribute__((nonnull));

/**
 * Analyze an upgrade_get_status command.
 * @param task Upgrade get status task to be processed.
 * @param error_code Variable to store an error code if something is wrong.
 * @return JSON object with the response for this task.
 * */
STATIC cJSON* wm_task_manager_command_upgrade_get_status(wm_task_manager_upgrade_get_status *task, int *error_code) __attribute__((nonnull));

/**
 * Analyze an upgrade_update_status command. Update the tasks DB when necessary.
 * @param task Upgrade update status task to be processed.
 * @param error_code Variable to store an error code if something is wrong.
 * @return JSON object with the response for this task.
 * */
STATIC cJSON* wm_task_manager_command_upgrade_update_status(wm_task_manager_upgrade_update_status *task, int *error_code) __attribute__((nonnull));

/**
 * Analyze an upgrade_result command.
 * @param task Upgrade result task to be processed.
 * @param error_code Variable to store an error code if something is wrong.
 * @return JSON object with the response for this task.
 * */
STATIC cJSON* wm_task_manager_command_upgrade_result(wm_task_manager_upgrade_result *task, int *error_code) __attribute__((nonnull));

/**
 * Analyze an upgrade_cancel_tasks command. Update the tasks DB when necessary.
 * @param task Upgrade cancel tasks task to be processed.
 * @param error_code Variable to store an error code if something is wrong.
 * @return JSON object with the response for this task.
 * */
STATIC cJSON* wm_task_manager_command_upgrade_cancel_tasks(wm_task_manager_upgrade_cancel_tasks *task, int *error_code) __attribute__((nonnull));

/**
 * Send messages to Wazuh DB.
 * @param command Command to be send.
 * @param parameters cJSON with the parameters
 * @param error_code Variable to store an error code if something is wrong.
 * @return JSON object with the response for this task.
 * */
STATIC cJSON* wm_task_manager_send_message_to_wdb(const char *command, cJSON *parameters, int *error_code) __attribute__((nonnull));

cJSON* wm_task_manager_process_task(const wm_task_manager_task *task, int *error_code) {
    cJSON *response = NULL;

    switch (task->command) {
    case WM_TASK_UPGRADE:
    case WM_TASK_UPGRADE_CUSTOM:
        response = wm_task_manager_command_upgrade((wm_task_manager_upgrade *)task->parameters, task->command, error_code);
        break;
    case WM_TASK_UPGRADE_GET_STATUS:
        response = wm_task_manager_command_upgrade_get_status((wm_task_manager_upgrade_get_status *)task->parameters, error_code);
        break;
    case WM_TASK_UPGRADE_UPDATE_STATUS:
        response = wm_task_manager_command_upgrade_update_status((wm_task_manager_upgrade_update_status *)task->parameters, error_code);
        break;
    case WM_TASK_UPGRADE_RESULT:
        response = wm_task_manager_command_upgrade_result((wm_task_manager_upgrade_result *)task->parameters, error_code);
        break;
    case WM_TASK_UPGRADE_CANCEL_TASKS:
        response = wm_task_manager_command_upgrade_cancel_tasks((wm_task_manager_upgrade_cancel_tasks *)task->parameters, error_code);
        break;
    default:
        *error_code = WM_TASK_INVALID_COMMAND;
    }

    return response;
}

STATIC cJSON* wm_task_manager_command_upgrade(wm_task_manager_upgrade *task, int command, int *error_code) {
    cJSON *response = cJSON_CreateArray();
    int agent_it = 0;
    int agent_id = 0;
    int task_id = OS_INVALID;

    while (agent_id = task->agent_ids[agent_it++], agent_id != OS_INVALID) {
        // Insert upgrade task into DB
        if (task_id = /*wm_task_manager_insert_task(agent_id, task->node, task->module, task_manager_commands_list[command])*/0, task_id == OS_INVALID) {
            *error_code = WM_TASK_DATABASE_ERROR;
            cJSON_Delete(response);
            break;
        } else {
            cJSON_AddItemToArray(response, wm_task_manager_parse_data_response(WM_TASK_SUCCESS, agent_id, task_id, NULL));
        }
    }

    return response;
}

STATIC cJSON* wm_task_manager_command_upgrade_get_status(wm_task_manager_upgrade_get_status *task, int *error_code) {
    cJSON *response = cJSON_CreateArray();
    int agent_it = 0;
    int agent_id = 0;
    int result = 0;
    char *status_result = NULL;

    while (agent_id = task->agent_ids[agent_it++], agent_id != OS_INVALID) {
        // Get upgrade task status
        if (result = /*wm_task_manager_get_upgrade_task_status(agent_id, task->node, &status_result)*/0, result == OS_INVALID) {
            *error_code = WM_TASK_DATABASE_ERROR;
            cJSON_Delete(response);
            break;
        } else {
            cJSON_AddItemToArray(response, wm_task_manager_parse_data_response(result, agent_id, OS_INVALID, status_result));
        }
    }

    os_free(status_result);

    return response;
}

STATIC cJSON* wm_task_manager_command_upgrade_update_status(wm_task_manager_upgrade_update_status *task, int *error_code) {
    cJSON *response = cJSON_CreateArray();
    int agent_it = 0;
    int agent_id = 0;
    int result = 0;

    while (agent_id = task->agent_ids[agent_it++], agent_id != OS_INVALID) {
        // Update upgrade task status
        if (result = /*wm_task_manager_update_upgrade_task_status(agent_id, task->node, task->status, task->error_msg)*/0, result == OS_INVALID) {
            *error_code = WM_TASK_DATABASE_ERROR;
            cJSON_Delete(response);
            break;
        } else {
            cJSON_AddItemToArray(response, wm_task_manager_parse_data_response(result, agent_id, OS_INVALID, task->status));
        }
    }

    return response;
}

STATIC cJSON* wm_task_manager_command_upgrade_result(wm_task_manager_upgrade_result *task, int *error_code) {
    cJSON *response = cJSON_CreateArray();
    int agent_it = 0;
    int agent_id = 0;
    int create_time = OS_INVALID;
    int last_update_time = OS_INVALID;
    char *node_result = NULL;
    char *module_result = NULL;
    char *command_result = NULL;
    char *status = NULL;
    char *error = NULL;
    int task_id = OS_INVALID;

    while (agent_id = task->agent_ids[agent_it++], agent_id != OS_INVALID) {
        if (task_id = /*wm_task_manager_get_upgrade_task_by_agent_id(agent_id, &node_result, &module_result, &command_result, &status, &error, &create_time, &last_update_time)*/0, task_id == OS_INVALID) {
            *error_code = WM_TASK_DATABASE_ERROR;
            cJSON_Delete(response);
            break;
        } else if (task_id == OS_NOTFOUND || task_id == 0) {
            cJSON_AddItemToArray(response, wm_task_manager_parse_data_response(WM_TASK_DATABASE_NO_TASK, agent_id, OS_INVALID, NULL));
        } else {
            cJSON *tmp = wm_task_manager_parse_data_response(WM_TASK_SUCCESS, agent_id, task_id, NULL);
            wm_task_manager_parse_data_result(tmp, node_result, module_result, command_result, status, error, create_time, last_update_time, task_manager_commands_list[WM_TASK_UPGRADE_RESULT]);
            cJSON_AddItemToArray(response, tmp);
        }
    }

    os_free(node_result);
    os_free(module_result);
    os_free(command_result);
    os_free(status);
    os_free(error);

    return response;
}

STATIC cJSON* wm_task_manager_command_upgrade_cancel_tasks(wm_task_manager_upgrade_cancel_tasks *task, int *error_code) {
    cJSON *response = NULL;
    cJSON *parameters = cJSON_CreateObject();
    cJSON *wdb_response = NULL;

    cJSON_AddStringToObject(parameters, task_manager_json_keys[WM_TASK_NODE], task->node);

    // Cancel pending tasks for this node
    if (wdb_response = wm_task_manager_send_message_to_wdb(task_manager_commands_list[WM_TASK_UPGRADE_CANCEL_TASKS], parameters, error_code), wdb_response) {

        cJSON *wdb_error = cJSON_GetObjectItem(wdb_response, task_manager_json_keys[WM_TASK_ERROR]);

        if (wdb_error && (wdb_error->type == cJSON_Number) && (wdb_error->valueint == OS_SUCCESS)) {
            response = wm_task_manager_parse_data_response(WM_TASK_SUCCESS, OS_INVALID, OS_INVALID, NULL);
        }
        cJSON_Delete(wdb_response);
    }
    cJSON_Delete(parameters);

    return response;
}

void* wm_task_manager_clean_tasks(void *arg) {
    wm_task_manager *config = (wm_task_manager *)arg;
    time_t next_clean = time(0);
    time_t next_timeout = time(0);

    while (1) {
        time_t now = time(0);
        time_t sleep_time = 0;

        if (now >= next_timeout) {
            // Set the status of old tasks IN PROGRESS to TIMEOUT
            cJSON *parameters = cJSON_CreateObject();
            cJSON *wdb_response = NULL;
            int error_code = WM_TASK_SUCCESS;

            cJSON_AddNumberToObject(parameters, task_manager_json_keys[WM_TASK_NOW], now);
            cJSON_AddNumberToObject(parameters, task_manager_json_keys[WM_TASK_TIMESTAMP], config->task_timeout);

            // Set next timeout
            next_timeout = now + config->task_timeout;

            if (wdb_response = wm_task_manager_send_message_to_wdb(task_manager_commands_list[WM_TASK_SET_TIMEOUT], parameters, &error_code), wdb_response) {

                cJSON *wdb_error = cJSON_GetObjectItem(wdb_response, task_manager_json_keys[WM_TASK_ERROR]);

                if (wdb_error && (wdb_error->type == cJSON_Number) && (wdb_error->valueint == OS_SUCCESS)) {
                    cJSON *timeout_json = cJSON_GetObjectItem(wdb_response, task_manager_json_keys[WM_TASK_TIMESTAMP]);

                    if (timeout_json && (timeout_json->type == cJSON_Number)) {
                        // Update next timeout
                        next_timeout = timeout_json->valueint;
                    }
                }
                cJSON_Delete(wdb_response);
            }
            cJSON_Delete(parameters);
        }

        if (now >= next_clean) {
            // Delete entries older than cleanup_time
            cJSON *parameters = cJSON_CreateObject();
            cJSON *wdb_response = NULL;
            int error_code = WM_TASK_SUCCESS;

            cJSON_AddNumberToObject(parameters, task_manager_json_keys[WM_TASK_TIMESTAMP], (now - config->cleanup_time));

            // Set next clean time
            next_clean = now + WM_TASK_CLEANUP_DB_SLEEP_TIME;

            if (wdb_response = wm_task_manager_send_message_to_wdb(task_manager_commands_list[WM_TASK_DELETE_OLD], parameters, &error_code), wdb_response) {
                cJSON_Delete(wdb_response);
            }
            cJSON_Delete(parameters);
        }

        if (next_timeout < next_clean) {
            sleep_time = next_timeout;
        } else {
            sleep_time = next_clean;
        }

        w_sleep_until(sleep_time);

    #ifdef WAZUH_UNIT_TESTING
        break;
    #endif
    }

    return NULL;
}

STATIC cJSON* wm_task_manager_send_message_to_wdb(const char *command, cJSON *parameters, int *error_code) {
    cJSON *response = NULL;
    const char *json_err;
    int result = 0;
    char *parameters_in_str = NULL;
    char wdbquery[WDBQUERY_SIZE] = "";
    char wdboutput[WDBOUTPUT_SIZE] = "";
    char *payload = NULL;
    int socket = -1;

    parameters_in_str = cJSON_PrintUnformatted(parameters);
    snprintf(wdbquery, sizeof(wdbquery), "task %s %s", command, parameters_in_str);
    os_free(parameters_in_str);

    result = wdbc_query_ex(&socket, wdbquery, wdboutput, sizeof(wdboutput));
    wdbc_close(&socket);

    if (result == OS_SUCCESS) {
        if (WDBC_OK == wdbc_parse_result(wdboutput, &payload)) {
            if (response = cJSON_ParseWithOpts(payload, &json_err, 0), !response) {
                mterror(WM_TASK_MANAGER_LOGTAG, MOD_TASK_PARSE_JSON_ERROR, payload);
                *error_code = WM_TASK_DATABASE_PARSE_ERROR;
            }
        } else {
            mterror(WM_TASK_MANAGER_LOGTAG, MOD_TASK_TASKS_DB_ERROR_IN_QUERY, payload);
            *error_code = WM_TASK_DATABASE_REQUEST_ERROR;
        }
    } else {
        mterror(WM_TASK_MANAGER_LOGTAG, MOD_TASK_TASKS_DB_ERROR_EXECUTE, WDB_TASK_DIR, WDB_TASK_NAME);
        *error_code = WM_TASK_DATABASE_ERROR;
    }

    return response;
}

#endif
