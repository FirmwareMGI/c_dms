/*
 * client_example_control.c
 *
 * How to control a device ... intended to be used with server_example_control
 */

#include "iec61850_client.h"
#include "hal_thread.h"

#include <stdlib.h>
#include <stdio.h>

//cok
static void commandTerminationHandler(void *parameter, ControlObjectClient connection)
{


    LastApplError lastApplError = ControlObjectClient_getLastApplError(connection);

    /* if lastApplError.error != 0 this indicates a CommandTermination- */
    if (lastApplError.error != 0) {
        printf("Received CommandTermination-.\n");
        printf(" LastApplError: %i\n", lastApplError.error);
        printf("      addCause: %i\n", lastApplError.addCause);
    }
    else
        printf("Received CommandTermination+.\n");
}

int main(int argc, char** argv) {

    char* hostname;
    int tcpPort = 102;

    if (argc > 1)
        hostname = argv[1];
    else
        hostname = "192.168.2.7";

    if (argc > 2)
        tcpPort = atoi(argv[2]);

    IedClientError error;

    IedConnection con = IedConnection_create();

    IedConnection_connect(con, &error, hostname, tcpPort);

    if (error == IED_ERROR_OK)
    {
        printf("Client Connected\n");
        MmsValue* ctlVal = NULL;
        MmsValue* stVal = NULL;
        printf("Connected to %s:%d\n", hostname, tcpPort);
        printf("Control object:");

        /************************
         * Direct control
         ***********************/
        ControlObjectClient control
         = NULL;
        // ControlObjectClient control
        //     = ControlObjectClient_create("BCUCPLCONTROL1/CSWI1.Pos", con);
        control = ControlObjectClient_create("BCUCPLCONTROL1/CSWI9.Pos", con);

        if (control)
        {
            
            ControlObjectClient_setCommandTerminationHandler(control, commandTerminationHandler, NULL);

            ctlVal = MmsValue_newBoolean(false);

            if (ControlObjectClient_selectWithValue(control, ctlVal)) {

                if (ControlObjectClient_operate(control, ctlVal, 0 /* operate now */)) {
                    printf("IEDCTRL/GGIO1.Ind1 operated successfully\n");
                }
                else {
                    printf("failed to operate IEDCTRL/GGIO1.Ind1!\n");
                }

            }
            else {
                printf("failed to select IEDCTRL/GGIO1.Ind1!\n");
            }

            MmsValue_delete(ctlVal);

            /* Wait for command termination message */
            Thread_sleep(1000);

            ControlObjectClient_destroy(control);
        }
        else {
            printf("Control object IEDCTRL/GGIO1.Ind1 not found in server\n");
        }

    //     /*********************************************************************
    //      * Direct control with enhanced security (expect CommandTermination-)
    //      *********************************************************************/

    //     control = ControlObjectClient_create("IEDCTRL/GGIO1.Ind1", con);

    //     if (control)
    //     {
    //         ControlObjectClient_setCommandTerminationHandler(control, commandTerminationHandler, NULL);

    //         ctlVal = MmsValue_newBoolean(true);

    //         if (ControlObjectClient_operate(control, ctlVal, 0 /* operate now */)) {
    //             printf("IEDCTRL/GGIO1.Ind1 operated successfully\n");
    //         }
    //         else {
    //             printf("failed to operate IEDCTRL/GGIO1.Ind1\n");
    //         }

    //         MmsValue_delete(ctlVal);

    //         /* Wait for command termination message */
    //         Thread_sleep(1000);

    //         ControlObjectClient_destroy(control);
    //     }
    //     else {
    //         printf("Control object IEDCTRL/GGIO1.Ind1 not found in server\n");
    //     }

    //     IedConnection_close(con);
    // }
    // else {
    // 	printf("Connection failed!\n");
    // }

    IedConnection_destroy(con);
    return 0;
    }
}


