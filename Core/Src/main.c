/* USER CODE BEGIN Header */
/** FILE          : main.c
 PROJECT       : PROG8125
 PROGRAMMER    : R. Hofer
 FIRST VERSION : 2016-02-0
 REVISED:	   : 2017-01-23 - Changed to run on the Nucleo-32 and in TrueSTUDIO
 REVISED:	   : 2019-03-27 Allan Smith - Changed to work with v 9.20 of TrueSTUDIO and cubemx32 5.1
 	 	 	 	 and fix a number of minor issues
 	 	 	 	 2020-11-18 Allan Smith - fixed to work with CubeMx ide 1.4.2
 REVISED:      : 2021-08-01 Sagal Singh kalra -
 	 	 	 	 Continued on the partially completed program to finish all the states of the debit machine.
  	  			 Added Chequing and Savings account, transaction is approved only when both accounts have enough balance.
  	  			 An audio tone is played for 500ms if the transaction is approved or declined from the bank. A different
  	  			 tone is played if the transaction is cancelled by the user.
  	  			 Used linked lists to store the data or record the transaction along with date and time.
  	  			 Asks the user to print receipt after successful transaction.
 DESCRIPTION   : Demonstrates a debit machine banking transaction that implements a state machine.
 	 	 	 	 State machine diagram is found on slide 14 of Week 9 Switch Statement and State Machine.pptx

 Switches are assigned as follows
 note: these pins are set in the debounceInit functions and do not need to be configured in cube
 PA0			PA1			PA4			PA3
 chequing		savings		ok			cancel

 Note: Don't use PA2 as it is connected to VCP TX and you'll
 lose printf output ability.


 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <unistd.h>
#include <stdio.h>
#include<stdlib.h>
#include "debounce.h"
#include "HD44780.h"
#include <string.h>
#include<time.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
RTC_HandleTypeDef hrtc;

TIM_HandleTypeDef htim1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
static const int16_t chequingPbPin = 0; //setting the pin assigned to each pb
static const int16_t savingsPbPin = 1;		//don't use pin 2 as it's connected
static const int16_t okPbPin = 4;		//to VCP TX
static const int16_t cancelPbPin = 3;

enum pushButton {
	none, chequing, savings, ok, cancel
};
//enumerated values for use with if
//(pbPressed == value) type conditional
//statements

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM1_Init(void);
static void MX_RTC_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// FUNCTION      : setTone
// DESCRIPTION   : Calculates the PWM Period needed to obtain the freq
//				 : passed and the duty cycle of the PAM to
//				 : 50% (1/2 of the period)
// PARAMETERS    : int32 freq - frequency of the output
// RETURNS       : nothing
static void changeSpeakerFrequency(TIM_HandleTypeDef *htim,
		uint32_t newFrequency) {

	//HAL_TIMEx_PWMN_Stop(htim, TIM_CHANNEL_1);

	// calculate the new period based off of frequency input
	uint32_t newPeriod = 1000000000 / (newFrequency * 250);

	TIM_ClockConfigTypeDef sClockSourceConfig = { 0 };
	TIM_MasterConfigTypeDef sMasterConfig = { 0 };
	TIM_OC_InitTypeDef sConfigOC = { 0 };
	TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = { 0 };

	htim->Instance = TIM1;
	htim->Init.Prescaler = 0;
	htim->Init.CounterMode = TIM_COUNTERMODE_UP;
	htim->Init.Period = newPeriod;
	htim->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim->Init.RepetitionCounter = 0;
	htim->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(htim) != HAL_OK) {
		Error_Handler();
	}
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(htim, &sClockSourceConfig) != HAL_OK) {
		Error_Handler();
	}
	if (HAL_TIM_PWM_Init(htim) != HAL_OK) {
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(htim, &sMasterConfig)
			!= HAL_OK) {
		Error_Handler();
	}
	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.Pulse = htim1.Init.Period/2;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
	sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
	if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1)
			!= HAL_OK) {
		Error_Handler();
	}
	sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
	sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
	sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
	sBreakDeadTimeConfig.DeadTime = 0;
	sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
	sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
	sBreakDeadTimeConfig.BreakFilter = 0;
	sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;
	sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
	sBreakDeadTimeConfig.Break2Filter = 0;
	sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
	if (HAL_TIMEx_ConfigBreakDeadTime(htim, &sBreakDeadTimeConfig)
			!= HAL_OK) {
		Error_Handler();
	}
	HAL_TIM_MspPostInit(htim);

	// must restart the timer once changes are complete
	HAL_TIMEx_PWMN_Start(htim, TIM_CHANNEL_1);
}

// FUNCTION      : waitForPBRelease
// DESCRIPTION   : Loops until the PB that is currently
//				 : pressed and at a logic low
//				 : is released. Release is debounced
// PARAMETERS    : pin - pin number
//                 port- port letter ie 'A'
// RETURNS       : nothing
void waitForPBRelease(const int16_t pin, const char port) {
	while (deBounceReadPin(pin, port, 10) == 0) {
		//do nothing wait for key press to be released
	}
}

// FUNCTION      : startUpLCDSplashScreen()
// DESCRIPTION   : displays Debit Demo for 2s
//                 on line 1 of the display and
//				 : Disappears
// PARAMETERS    : None
// RETURNS       : nothing
void startUpLCDSplashScreen(void) {
	char stringBuffer[16] = { 0 };
	HD44780_GotoXY(0, 0);
	snprintf(stringBuffer, 16, "   Debit Demo");
	HD44780_PutStr(stringBuffer);
	HAL_Delay(2000);
	HD44780_ClrScr();
}

// FUNCTION      : pulsePWM
// DESCRIPTION   : Turns on the PWM for the pulseTime in ms
//                 provided and then turns off PWM
// PARAMETERS    : address of Timer Handle var (e.g.: &htim1)
//                 pulseTime in ms
// RETURNS       : nothing
void playAudio(uint32_t frequency, uint32_t duration){
	__HAL_TIM_SET_AUTORELOAD(&htim1, frequency);
	HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
	changeSpeakerFrequency(&htim1, frequency);
	HAL_Delay(duration);
	HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_1);
}

//  FUNCTION      : pushButtonInit
//   DESCRIPTION   : Calls deBounceInit to initialize ports that
//                   will have pushbutton on them to be inputs.
//			         Initializing PA0,PA1,PA4 and PA3
//                   Switches are assigned as follows
//                   PA0			PA1			PA4			PA3
//                   chequing		savings		ok			cancel
//
//                   Note: Don't use PA2 as it is connected to VCP TX and you'll
//                   lose printf output ability.
//   PARAMETERS    : None
//   RETURNS       : nothing
void pushButtonInit(void) {
	deBounceInit(chequingPbPin, 'A', 1); 		//1 = pullup resistor enabled
	deBounceInit(savingsPbPin, 'A', 1); 		//1 = pullup resistor enabled
	deBounceInit(okPbPin, 'A', 1); 			//1 = pullup resistor enabled
	deBounceInit(cancelPbPin, 'A', 1); 		//1 = pullup resistor enabled
}

// FUNCTION      : displayWelcome()
// DESCRIPTION   : clears the LCD display and displays
//                 Welcome on line 1 of the display
// PARAMETERS    : None
// RETURNS       : nothing
void displayWelcome(void) {
	char stringBuffer[16] = { 0 };
	HD44780_ClrScr();
	snprintf(stringBuffer, 16, "Welcome ");
	HD44780_PutStr(stringBuffer);
}

// FUNCTION      : displayAmount()
// DESCRIPTION   : clears the LCD display and displays
//                 the $amount received on line 1 of the display
// PARAMETERS    : float - amount to display
// RETURNS       : nothing
void displayAmount(float amount) {
	char stringBuffer[16] = { 0 };
	HD44780_ClrScr();
	snprintf(stringBuffer, 16, "$%.2f", amount);
	HD44780_PutStr(stringBuffer);
}

// FUNCTION      : checkIfAmountRecd()
// DESCRIPTION   :
// PARAMETERS    : none
// RETURNS       : float, the amount in $ to be debited
float checkIfAmountRecd() {
	float debitAmount = 0;
	printf("waiting for debitAmount to be received on serial port\r\n");
	int16_t result = 0;
	result = scanf("%f", &debitAmount);
	if (result == 0)		//then somehow non-float chars were entered
	{						//and nothing was assigned to %f
		fpurge(STDIN_FILENO); //clear the last erroneous char(s) from the input stream
	}
	return debitAmount;
}

// FUNCTION      : checkOkOrCancel()
// DESCRIPTION   : Checks whether the OK or Cancel
//                 button has been pressed.
// PARAMETERS    : none
// RETURNS       : int8_t, 3 if cancel pressed, 4 if ok
//                 ok pressed. 0 returned if neither
//                 has pressed.
enum pushButton checkOkOrCancel(void) {
	if (deBounceReadPin(cancelPbPin, 'A', 10) == 0) {
		//then the cancel pushbutton has been pressed
		return cancel;
	} else if (deBounceReadPin(okPbPin, 'A', 10) == 0) {
		//then ok pressed
		return ok;
	}
	return none; //as ok or cancel was not pressed.
}

// FUNCTION      : displayOkOrCancel()
// DESCRIPTION   : displays "OK or Cancel?" on line 2 of LCD
// PARAMETERS    : none
// RETURNS       : nothing.
void displayOkCancel(void) {
	char stringBuffer[16] = { 0 };
	HD44780_GotoXY(0, 1); //move to second line first position
	snprintf(stringBuffer, 16, "OK or Cancel?");
	HD44780_PutStr(stringBuffer);
}

// FUNCTION      : chequingOrSavings()
// DESCRIPTION   : Checks weather chequing or Savings
//                 is pressed.
// PARAMETERS    : none
// RETURNS       : Returns 0 if chequing is pressed.
//                 Returns 1 if savings is pressed.
enum pushButton chequingOrSavings(void){
	if(deBounceReadPin(chequingPbPin, 'A', 10) == 0){
		return chequing;
	}else if(deBounceReadPin(savingsPbPin, 'A', 10) == 0){
		return savings;
	}
	return none;
}

// FUNCTION      : displaySavingOrChequing()
// DESCRIPTION   : displays "Chequing" or "Savings"?
//                 "Chequing" on first line of LCD
//                 "Saving" on second line of LCD
// PARAMETERS    : none
// RETURNS       : nothing.
void displaySavingOrChequing(void){
	char string1[16] = {0};
	char string2[16] = {0};
	HD44780_ClrScr();
	snprintf(string1, 16, "Chequing or");
	HD44780_GotoXY(0, 0);
	HD44780_PutStr(string1);
	snprintf(string2, 16, "Savings?");
	HD44780_GotoXY(0, 1);
	HD44780_PutStr(string2);
}

// FUNCTION      : displayTransactionCancel()
// DESCRIPTION   : displays "Transaction Cancelled"
//                 on 1st and 2nd line of LCD respectively.
//                 Calls the function displayWelcome() after
//				   a delay of 1500ms.
// PARAMETERS    : none
// RETURNS       : nothing.
void displayTransactionCancel(void){
	char string1[16] = {0};
	char string2[16] = {0};
	HD44780_ClrScr();
	snprintf(string1, 16, "Transaction");
	HD44780_GotoXY(0, 0);
	HD44780_PutStr(string1);
	snprintf(string2, 16, "Cancelled");
	HD44780_GotoXY(0, 1);
	HD44780_PutStr(string2);
	playAudio(600, 500);
	HAL_Delay(1500);
	displayWelcome();

}

// FUNCTION      : displayEnterPIN()
// DESCRIPTION   : displays "ENTER PIN" to ask the
//                 user to enter the PIN.
// PARAMETERS    : none
// RETURNS       : nothing.
void displayEnterPIN(void){
	char string1[16] = { 0 };
	HD44780_ClrScr();
	snprintf(string1, 16, "Enter PIN");
	HD44780_GotoXY(0, 0);
	HD44780_PutStr(string1);
}

// FUNCTION      : enterPIN()
// DESCRIPTION   : Checks if the debit PIN entered by the
//                 user matches the saved password.
// PARAMETERS    : none
// RETURNS       : Returns 1 if the password matches.
//               : Returns 0 if the password doesn't match.
int enterPIN(void){
	char password[] = "1234";
	char inputPassword[5] = {'\0'};
	printf("Enter PIN\r\n");
	scanf("%s", inputPassword);
	printf("Password:\r\n");
	puts(inputPassword);
	if(strcmp(password, inputPassword) == 0){
		return 1;
	}
	else
		return 0;
}

// FUNCTION      : bankOK()
// DESCRIPTION   : mimics and displays the sequence on LCD of a
//                 debit machine when transaction is APPROVED.
//                 Shows that the machine is connected to the bank
//                 and returns confirmation as an approval.
// PARAMETERS    : none
// RETURNS       : nothing.
void bankOK(void){
	HD44780_ClrScr();
	HD44780_PutStr("waiting..");
	HAL_Delay(500);
	HD44780_ClrScr();
	HD44780_PutStr("connected..");
	HAL_Delay(500);
	HD44780_ClrScr();
	HD44780_PutStr("APPROVED");
	playAudio(400, 500);
}

// FUNCTION      : bankNotOK()
// DESCRIPTION   : mimics and displays the sequence on LCD of a
//                 debit machine when transaction is DECLINED.
//                 Shows that the machine is connected to the bank
//                 server and returns DECLINED if balance is low.
//                 After 1000ms function displayTransactionCancel()
//                 is called.
// PARAMETERS    : none
// RETURNS       : nothing.
void bankNotOK(void){
	HD44780_ClrScr();
	HD44780_PutStr("waiting..");
	HAL_Delay(500);
	HD44780_ClrScr();
	HD44780_PutStr("connected..");
	HAL_Delay(500);
	HD44780_ClrScr();
	HD44780_PutStr("DECLINED");
	playAudio(600, 500);
	HAL_Delay(1000);
	displayTransactionCancel();
}

// FUNCTION      : displayPrintReciept()
// DESCRIPTION   : displays "Print Receipt?" on line 1 of LCD
//                 displays "YES   NO" on line 2 of LCD
// PARAMETERS    : none
// RETURNS       : nothing.
void displayPrintReciept(void){
	HD44780_ClrScr();
	HD44780_PutStr("Print Receipt?");
	HD44780_GotoXY(0, 1);
	HD44780_PutStr("YES   NO");
}

// FUNCTION      : displayPrintingReciept()
// DESCRIPTION   : displays "Printing..."
//                 After 2000ms displays "Thank You"
//                 After 700ms function displayWelcome()
// PARAMETERS    : none
// RETURNS       : nothing.
void displayPrintingReciept(void){
	HD44780_ClrScr();
	HD44780_PutStr("Printing...");
	HAL_Delay(2000);
	HD44780_ClrScr();
	HD44780_PutStr("Thank You");
	HAL_Delay(700);
	displayWelcome();
}

// STRUCTURE     : node
// DESCRIPTION   : data structure represents node of
//                 a linked list
// PARAMETERS    : debitAmount
//                 node* next
struct node{
	float debitAmount;
	struct node* next;
};

// FUNCTION      : printTransactionRecord()
// DESCRIPTION   : Print transaction record in a linked list along
//                 with date and time of transaction and amount.
// PARAMETERS    : node* head - points to the node of linked list
// RETURNS       : nothing.
// Reference     : https://www.geeksforgeeks.org/time-h-header-file-in-c-with-examples/
void printTransactionRecord(struct node* head){
	time_t t;
	time(&t);
	while(head){
		printf("DEBIT/TRANSACTION/%s/$%.2f\r\n",ctime(&t), head->debitAmount);
		head = head->next;
	}
}

// FUNCTION      : recordTransactionEndOfList()
// DESCRIPTION   : Record each transaction at the end of a linked list
// PARAMETERS    : node** head
//                 amountValue
// RETURNS       : nothing.
// Reference     : https://qnaplus.com/insert-element-singly-linked-list/
void recordTransactionEndOfList(struct node** head, float amountValue){
	    struct node* new_node = NULL;
	    struct node* last = NULL;       //Last node of the list points to NULL

	    new_node = (struct node*)malloc(sizeof(struct node));  //Creating new node

	    if (new_node == NULL)
	    {
	        printf("Failed to insert element. Out of memory");
	    }

	    new_node->debitAmount = amountValue;
	    new_node->next = NULL;

	    if (*head == NULL)
	    {
	        *head = new_node;
	        return;
	    }

	    last = *head;
	    while (last->next) last = last->next;

	    last->next = new_node;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_TIM1_Init();
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */

	printf("Debit Card State Machine\r\n");
	HD44780_Init();
	/* setup Port A bits 0,1,2 and 3, i.e.: PA0-PA3 for input */
	pushButtonInit();
	displayWelcome();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		float amount = 0;             				//used to hold the transaction amount
		static int8_t transactionState = 1;			//Transaction States
		static float getAmount = 0;					//
		static float accountBalanceSavings = 2000;	//Holds Balance in Savings in account
		static float accountBalanceChequing = 1000; //Holds Balance in Chequing account
		struct node* head = NULL;
		//int32_t pulseDuration = 0;
		enum pushButton pbPressed = none;  //will hold pushbutton defined above depending on
		//the pushbutton pressed
		/*states:
		 1   display Welcome Screen, wait for $ amount input from Serial port
		 2   @ amount Received, waiting for Ok or Cancel button
		 3   OK received, waiting for chequing or Savings button
		 4   C or S received, waiting for PIN to be entered from Serial Port
		 5   Send transaction data to bank. Waiting for OK back
		     from Bank If OK from Bank received. Record transaction.
		 6   Ask user to print receipt and if Cancel Pressed.
		 	 Display "Transaction Cancelled" back to state 1
		 */
		switch (transactionState) {
		case 1: 					//checking if an amount has been received
			amount = checkIfAmountRecd();
			if (amount != 0)        //returns a 0 if an transaction amount has
			{ 						//NOT been received on the serial port.
				displayAmount(amount); //but if we're we've received a debitAmount
				getAmount = amount;
				displayOkCancel();	//so display it and the prompt ok or cancel
				transactionState = 2;//and do that before we move on to state 2
			}
			break;
		case 2: 						//amount has been received waiting for
			pbPressed = checkOkOrCancel();

			if (pbPressed != none) {
				if (pbPressed == cancel) {
					//then cancel was pressed.
					printf("Cancel Pressed\r\n");
					transactionState = 1;
					displayTransactionCancel();
				} else if (pbPressed == ok) {
					//then ok pressed
					printf("OK Pressed\r\n");
					transactionState = 3;
					displaySavingOrChequing();
				}
			}
			break;

		case 3:
			pbPressed = chequingOrSavings();
			if(pbPressed != none){
				if(pbPressed == cancel){
					printf("Cancel Pressed\r\n");
					transactionState = 1;
					//If cancel Transaction Cancelled on LCD
					displayTransactionCancel();
				}
				else if(pbPressed == chequing){
					printf("Chequing Pressed\r\n");
					transactionState = 4;
					//Display to enter PIN
					displayEnterPIN();
				}
				else if(pbPressed == savings){
					printf("Savings Pressed\r\n");
					transactionState = 4;
					displayEnterPIN();
				}
			}
			break;

		case 4:

			/*Enter PIN via serial port and
			  check if PIN is correct
			  If PIN is incorrect state control
			  goes to transactionState 1 */
			if(enterPIN()){
				transactionState = 5;
			}
			else
			{
				bankNotOK();
				transactionState = 1;
			}
			break;

		case 5:
			/*Checks if Savings account has enough balance
			  Deducts and displays savings acc balance
			  Calls the func recordTransactionEndOfList() to
			  insert data at the end of list
			  Calls the func printTransactionRecord() to
			  display the linked list */
			if(accountBalanceSavings > getAmount){
				bankOK();
				displayPrintReciept();
				accountBalanceSavings -= getAmount;
				printf("Savings Account: %.2f\r\n",accountBalanceSavings);
				recordTransactionEndOfList(&head, getAmount);
				printTransactionRecord(head);
				transactionState = 6;
			}

			/*Checks if Chequing account has enough balance
			  Deducts and displays chequing acc balance
			  Calls the func recordTransactionEndOfList() to
			  insert data at the end of list
			  Calls the func printTransactionRecord() to
			  display the linked list */
			else if(accountBalanceChequing > getAmount){
				bankOK();
				displayPrintReciept();
				accountBalanceChequing -= getAmount;
				printf("Chequing Account: %.2f\r\n",accountBalanceChequing);
				recordTransactionEndOfList(&head, getAmount);
				printTransactionRecord(head);
				transactionState = 6;
			}
			else{
				transactionState = 1;
				bankNotOK();
			}
			break;

		case 6:
			/*If ok pressed, receipt is printed
			  If cancel pressed, flow of control goes back
			  to transactionState 1 and displays welcome again.
			 */
			pbPressed = checkOkOrCancel();
			if(pbPressed != none){
				if(pbPressed == ok){
					transactionState = 1;
					displayPrintingReciept();
				}else if(pbPressed == cancel){
					displayWelcome();
					transactionState = 1;
				}
			}
			break;
		default:
			break;
		} //closing brace for switch

	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_LSE
                              |RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 16;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC|RCC_PERIPHCLK_USART2;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Enable MSI Auto calibration
  */
  HAL_RCCEx_EnableMSIPLLMode();
}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */
  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */

  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 0x0;
  sTime.Minutes = 0x0;
  sTime.Seconds = 0x0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  sDate.Month = RTC_MONTH_JANUARY;
  sDate.Date = 0x1;
  sDate.Year = 0x0;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 100;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.BreakFilter = 0;
  sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;
  sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
  sBreakDeadTimeConfig.Break2Filter = 0;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_1|LD3_Pin|GPIO_PIN_4
                          |GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7, GPIO_PIN_RESET);

  /*Configure GPIO pins : PB0 PB1 LD3_Pin PB4
                           PB5 PB6 PB7 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|LD3_Pin|GPIO_PIN_4
                          |GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
