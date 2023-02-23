/*
 *  ECE- 315 WINTER 2023 - COMPUTER INTERFACING COURSE
 *
 *  Created on: 	Date:::5 February, 2021
 *  Modified on:    Date:::27 January, 2023
 *      Author: 	Shyama M. Gandhi
 *
 *  Lab 1 - Part 2 : Simple Octal Calculator
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "xparameters.h"
#include "xgpio.h"
#include "xscugic.h"
#include "xil_exception.h"
#include "xil_printf.h"

#include <stdio.h>
#include <stdlib.h>
#include "pmodkypd.h"
#include "sleep.h"
#include "xil_cache.h"
#include "math.h"

//three tasks : Input task, Compute task and Output task
static void vInputTask( void *pvParameters );
static void vComputeTask( void *pvParameters );
static void vOutputTask( void *pvParameters );

void DemoInitialize();
u32 SSD_decode(u8 key_value, u8 cathode);

PmodKYPD myDevice;

static TaskHandle_t xInputTask;
static TaskHandle_t xComputeTask;
static TaskHandle_t xOutputTask;

static QueueHandle_t xQueue1 = NULL;
static QueueHandle_t xQueue2 = NULL;

#define DEFAULT_KEYTABLE "0FED789C456B123A"

void DemoInitialize() {
   KYPD_begin(&myDevice, XPAR_AXI_GPIO_PMOD_KEYPAD_BASEADDR);
   KYPD_loadKeyTable(&myDevice, (u8*) DEFAULT_KEYTABLE);
}

//----------------------------------------------------
// MAIN FUNCTION
//----------------------------------------------------
int main (void)
{
  xil_printf("*************************\n");
  xil_printf("Calculator Ready!\n");
  xil_printf("*************************\n");

  xTaskCreate( vInputTask,					/* The function that implements the task. */
    			( const char * ) "Ip_task", /* Text name for the task, provided to assist debugging only. */
    			configMINIMAL_STACK_SIZE, 	/* The stack allocated to the task. */
    			NULL, 						/* The task parameter is not used, so set to NULL. */
    			tskIDLE_PRIORITY+1,			/* The task runs at the idle priority + 1. */
    			&xInputTask );

  xTaskCreate( vComputeTask,
    			( const char * ) "Comp_task",
				configMINIMAL_STACK_SIZE,
				NULL,
    			tskIDLE_PRIORITY+2,
    			&xComputeTask );

  xTaskCreate( vOutputTask,
    			( const char * ) "Comp_task",
				configMINIMAL_STACK_SIZE,
				NULL,
    			tskIDLE_PRIORITY+3,
    			&xOutputTask );

  /* Create the queue used by the tasks. */

  //queue between input task and compute task
  xQueue1 = xQueueCreate( 3,				/* There are three items in the queue, two operands and then operation using keypad */
		  	  sizeof( unsigned int ) );
  //queue between compute task and output task
  xQueue2 = xQueueCreate( 1,
		  	  sizeof ( unsigned int) );

  /* Check the queue was created. */
  configASSERT(xQueue1);
  configASSERT(xQueue2);
  DemoInitialize();

  vTaskStartScheduler();

  while(1);

  return 0;
}

/*-----------------------------------------------------------*/
static void vInputTask( void *pvParameters )
{

	for( ;; ) {
	   u16 keystate;
	   XStatus status, last_status = KYPD_NO_KEY;
	   u8 key, last_key = 'x';
	   unsigned current_value = 0;
	   unsigned int operand_store[3];
	   char operand[20];	//records key 0-7
	   int itr_queue=0;		//no. of elements for queue
	   int op_itr=0;		//no. of key press stored in "operand"

	   Xil_Out32(myDevice.GPIO_addr, 0xF);
	   xil_printf("Press any key on the Keypad...\r\n");
	   xil_printf("Key Press 0-7 are valid for octal operands\n\n");

	   while (1) {

	      // Capture state of each key
	      keystate = KYPD_getKeyStates(&myDevice);

	      // Determine which single key is pressed, if any
	      status = KYPD_getKeyPressed(&myDevice, keystate, &key);

	      // Print key detect if a new key is pressed or if status has changed
	      if (status == KYPD_SINGLE_KEY
	            && (status != last_status || key != last_key)) {
	         xil_printf("Key Pressed: %c\r\n", (char) key);
	         last_key = key;

	         //whenever 'F' is pressed, the aggregated number will be registered as an operand
	         if((char)key == 'F'){

	        	 /*******************************/
				 //the key press are being stored in "operand" char array.
	        	 //find a C function that can store this as a base 8 number into the variable "current_value"

//	        	current_value = atoi(operand);

	        	 //sscanf(operand,"%o",&current_value);
	        	 //strtol(const char *str, char **endptr, int base)
	        	 current_value = strtol(operand, NULL,8);

	        	 // 1 line of code

				 /*******************************/
	        	 printf("Operand = %o\n",current_value);
	        	 operand_store[itr_queue++] = current_value;
		         current_value = 0;
		         op_itr=0;

		         /*******************************/
		         //write one line of code that sets the "operand" to null.
		         memset(operand,0,20*sizeof(char));

		         /*******************************/
	         }
	         else if(key>='0' && key<='7'){
	        	 operand[op_itr++] = (char)key;
	         }
	         else if((itr_queue == 2) &&(
					 (char)key == 'A' ||
					 (char)key == 'B' ||
					 (char)key == 'C' )){
	        	 //once two operands are in the queue, the third value to the queue is to indicate the operation to be performed using A,B or C key
	        	 unsigned int key_32 = (unsigned int)key;
	        	 operand_store[2] = key_32;

	        	 /*****************************************/
	        	 //write the code to send the "operand_store" to the queue that has handle "xQueue1"


	        	 for (int i = 0; i < 3; i++) {
	        		 xQueueSendToBack (
	        		 xQueue1,
	        		 &operand_store[i],
	        		 0UL
	        		);

//	        		 printf("Being sent: %c \r\n", operand_store[i]);
				}




	        	 /*****************************************/
	        	 itr_queue=0;
	         }
	         else{
	        	 xil_printf("Press a valid key!!! Keys 8,9,D and E is not a part of this calculator...\n");
	         }
	      }
	      else if (status == KYPD_MULTI_KEY && status != last_status)
	         xil_printf("Error: Multiple keys pressed\r\n"); //this is valid whenever two or more keys are pressed together

	      last_status = status;
	      usleep(10000);
	   }
	}
}
/*-----------------------------------------------------------*/
static void vComputeTask( void *pvParameters )
{
	unsigned int store_operands[3];
	unsigned int result;

	while(1)
	{
		/***************************************/
		//write the code to receive the queue elements inside "store_operands" (it is declared in this task)

		/***************************************/


	  	 for (int i = 0; i < 3; i++) {
		        		 xQueueReceive (
		        		 xQueue1,
		        		 &store_operands[i],
						 portMAX_DELAY
		        		);

//		            printf("Received %d \r\n",store_operands[i]);
					}



		/***************************************/
		//write the logic for the operations to be performed inside each case.
		//you can store the result of the operation inside the variable "result"
		switch ((char)store_operands[2]) {
		case 'A': // Add
			xil_printf("Addition : \t");
			if (store_operands[0] > (UINT32_MAX - store_operands[1])){
				xil_printf("Not a valid calculation\r\n");
			}
			else{
			result = store_operands[1] + store_operands[0];
			}
			break;

		case 'B': // Subtract
			xil_printf("Subtraction : \t");
			if (store_operands[0] < store_operands[1]){
				xil_printf("Not a valid calculation\r\n");
			}
			else{
			result = store_operands[0] - store_operands[1];
			}
			break;

		case 'C': // Multiply
			xil_printf("Multiplication : \t");
			if (store_operands[0] > (UINT32_MAX / store_operands[1])){
				xil_printf("Not a valid calculation\r\n");
			}
			else{
			result = store_operands[1] * store_operands[0];
			}
			break;

		default:
			xil_printf("Bad operand, could not calculate result!\r\n");
		}
		/***************************************/


		/***************************************/
		//write one line of code to send the "result" via queue that has handle "xQueue2"



		xQueueSendToBack(
	    xQueue2,
	    &result,
	    0UL
	 );




		/***************************************/

	}
}

static void vOutputTask( void *pvParameters )
{
	unsigned int _result;

	while(1){

		/***************************************/
		//write one line of code to receive the result from the queue inside the variable "_result"


				        		 xQueueReceive (
				        		 xQueue2,
				        		 &_result,
				        		 portMAX_DELAY
				        		);


		//print the result in octal format and decimal format
		/***************************************/
printf("The result in Decimal is: %d \n",_result);
printf(" The result in Octal is: %o \r\n", _result);

	}
}
