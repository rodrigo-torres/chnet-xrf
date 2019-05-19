﻿#include "../Header.h"
#include "mainwindow.h"


extern double ZPosition;
extern QString KeyenceValue;
extern int serialK,serialZ;
extern int send_command(int chan,const char *comando, const char *parametri, int port);

extern bool noKeyence_init;       extern bool AutofocusOn;
extern string checkK;      

double AutofocusBuffer[5] = {0,0,0};

double Autofocus_value=0;  double Autofocus_average_value=0;



double NewPositionValue;

int AutofocusIndex=0;             int AutofocusStore=0;      int DistanceLevel;           int NewPosInt=1000;    

bool ValueInRange=false; bool RunTracking=false;

void MainWindow::Autofocus2() {
    if(AutofocusOn) {
        AutofocusOn=false;
        printf("Autofocuson=false\n");
        timerAutofocus->stop();
        ENABLE_TRACKING_checkBox->setEnabled(false);
    }
    else {
        AutofocusOn=true;
        printf("Autofocuson=true\n");
        timerAutofocus->start(500);
        ENABLE_TRACKING_checkBox->setEnabled(true);
    }
}


/* This function is never called in the main or external programs
char *read_Kanswer()
{
  char c[100];
  int n=0;
  string rest;
  while((n=read(serialK, &c, sizeof(c)))>0) {
      c[n]=0;
      rest=rest+c;
      if ( c[n-1] == '\n' ) {
          break;
      }
  }
  return c;  
}
*/

string read_Kanswer2() {
  char c[12];
  int n=0;
  string restK;
  while ( ( n = read(serialK, &c, sizeof(c)) ) > 0 )
    {
      c[n]=0;
      restK=restK+c;      
      if ( c[n-1] == '\n' ) {
          break;
      }
    }
  return restK;
}


void MainWindow::readKeyence() {

    int counter_readKeyence = 0;
    while ( counter_readKeyence < 1) {
        qDebug()<<counter_readKeyence;
        checkK = read_Kanswer2();
        KeyenceValue="";
        KeyenceValue.append(checkK.data());
        KeyenceValue.truncate(9);
        Autofocus_value=KeyenceValue.toDouble();

        if ( Autofocus_value == 0) {
            qDebug()<<"... Keyence busy";
        }

        else {
            Autofocus_value=-((115226-Autofocus_value)/408.345);
            Autofocus_average_value=Autofocus_value;
            counter_readKeyence++;
        }
    }

    //double sum = 0 ;

    /*
    for ( int i=0; i < 3; ++i) {
        qDebug()<<i;
        qDebug()<<AutofocusBuffer[i];
        sum += AutofocusBuffer[i];
    }

    qDebug()<<Autofocus_average_value;
    Autofocus_average_value = sum / (double) 3;
    qDebug()<<Autofocus_average_value;
    */

    //Here we have to modify the code to account for the new geometry.

    //qDebug()<<"... Uncalibrated autofocus value is: "<< Autofocus_value;
    //The values for this formula come from a calibration of the instrument specific to the geometry of the detector head




   if ( (Autofocus_average_value <= (15.0)) && ( Autofocus_average_value > (-15.0)) ) {
       qDebug()<<Autofocus_average_value;
       ValueInRange=true;
       QString valueAsString = QString::number(Autofocus_average_value);
       lineEdit_2_tab_4->setText(valueAsString);

       /*
       switch (AutofocusIndex) {
       case 0:
           AutofocusBuffer[0]=Autofocus_value;
           AutofocusIndex++;
           break;
       case 1:
           AutofocusBuffer[1]=Autofocus_value;
           AutofocusIndex++;
           break;
       case 2:
           AutofocusIndex=0;
           Autofocus_average_value=(AutofocusBuffer[0]+AutofocusBuffer[1])/2;
           QString valueAsString = QString::number(Autofocus_average_value);
           lineEdit_2_tab_4->setText(valueAsString);
           break;
       break;
       }
       */

       /* Protocol for AutofocusIndex >= 2 move inside the switch statement
       if(AutofocusIndex>=2) {
           AutofocusIndex=0;
       }
       // fine controllo sui dati contenuti in +/- 15mm dallo zero del sensore
       */
   }

   else {
       qDebug()<<"[!] Laser out of range, autofocus value: "<<Autofocus_average_value;
       ValueInRange=false;
       lineEdit_2_tab_4->setText("[!] Out of range");
   }
   /* The boolean ReadyForTracking is set to true by default, and there's no function or declaration which changes its value.
    * The if statement below has no use.
   if(ReadyForTracking) {
           Autofocus_average_value=(AutofocusBuffer[0]+AutofocusBuffer[1])/2;
           QString valueAsString = QString::number(Autofocus_average_value);
           lineEdit_2_tab_4->setText(valueAsString);
   }
   */
   return;
}



void MainWindow::AutoFocusRunning() {

    if(RunTracking) {
        if(ValueInRange) {
            NewPositionValue=(ZPosition+Autofocus_average_value);
            DistanceLevel=abs(qRound((Autofocus_average_value*1000)));

            char v[10];

            /* [Doc.]   The following modifies the velocity of the Z-motor accordint to the difference between the current Z-motor position,
             *          and the desired Z-motor position. Then issues a command to the Z-motor to move with the specified velocity to the calculated
             *          position.
             */
            if(DistanceLevel>=5000)                         { sprintf(v, "%f", 5.0); send_command(1, "VEL", v,serialZ); }
            if((DistanceLevel>=3000)&&(DistanceLevel<5000)) { sprintf(v, "%f", 3.0); send_command(1, "VEL", v,serialZ); }
            if((DistanceLevel>=1000)&&(DistanceLevel<3000)) { sprintf(v, "%f", 1.2); send_command(1, "VEL", v,serialZ); }
            if((DistanceLevel>=500)&&(DistanceLevel<1000))  { sprintf(v, "%f", 0.5); send_command(1, "VEL", v,serialZ); }
            if((DistanceLevel>=200)&&(DistanceLevel<500))   { sprintf(v, "%f", 0.1); send_command(1, "VEL", v,serialZ); }
            if (DistanceLevel < 200)	                       { sprintf(v, "%f", 0.04); send_command(1, "VEL", v,serialZ);}

            NewPosInt=qRound(NewPositionValue*1000);

            if(DistanceLevel>=50) {
                AutofocusStore=NewPosInt;
                char sx[100];
                sprintf(sx,"%f",NewPositionValue);
                send_command(1,"MOV",sx,serialZ);
            }
        }
    }
}


void MainWindow::TrackingON() {
    if(RunTracking) {
        RunTracking=false;
        qDebug()<<"OFF";
        MOVE_Z_FORWARD_pushButton->setEnabled(true);
        MOVE_Z_BACKWARD_pushButton->setEnabled(true);
        MOVE_Z_To_pushButton->setEnabled(true);
        MOVE_Z_To_doubleSpinBox->setEnabled(true);
        MOVE_Z_MOTOR_label_1->setEnabled(true);
        MOVE_Z_MOTOR_label_2->setEnabled(false);
    }
    else {
        RunTracking=true;
        qDebug()<<"ON";
        MOVE_Z_FORWARD_pushButton->setEnabled(false);
        MOVE_Z_BACKWARD_pushButton->setEnabled(false);
        MOVE_Z_To_pushButton->setEnabled(false);
        MOVE_Z_To_doubleSpinBox->setEnabled(false);
        MOVE_Z_MOTOR_label_1->setEnabled(false);
        MOVE_Z_MOTOR_label_2->setEnabled(false);
    }
}

//int counter_FocusTimer = 0;

void MainWindow::Focustimer() {

    if ( noKeyence_init ) {
        Init_KeyenceLaser();
    }

    readKeyence();
    AutoFocusRunning();

    /*
    if ( counter_FocusTimer == 0 || counter_FocusTimer == 1) {
        readKeyence();
        counter_FocusTimer++;
    }

    if ( counter_FocusTimer == 2) {
        AutoFocusRunning();
        counter_FocusTimer=0;
    }
    */
}




