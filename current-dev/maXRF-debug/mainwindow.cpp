#include "../Header.h"
#include "mainwindow.h"
#include "../variables.h"
#include <unistd.h>
#include <QThread>
#include <tty.h>

extern int shmid[];
extern double Px;

tty *tty_ptr;

bool MapIsOpened = false;
bool CameraOn = false;
bool energychanged = false;

extern bool stage_on_target[3];
bool stage_on_target[] = { false };

bool XYscanning = false, YHasMoved = true;

bool AutofocusOn=false;

int measuring_time = 300; // for single-spectrum DAQ
int point, OffsetX, OffsetY, Pixeldim = 1; // for XRF map display

double positionX = 100, positionY = 100, scan_velocity = 1, time_per_pixel = 1000; // for scan limits
double ZPosition = 25.0;
double ChMin = 3, ChMax = 16384; // for spectrum scale
double Pz = 250;
double x_image, y_image, x_image2, y_image2;

char process[30];

struct Pixel_BIG *Pointer; //puntatore da far puntare a PixelsMappa una volta creato

double ChMin1=0, ChMax1=0, ChMin2=0, ChMax2=0, ChMin3=0, ChMax3=0;


QString stylesheet3 = "QLineEdit {background-color: #2DC937; font-weight: bold; color: white;}";




MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {

    PixelX=595;
    PixelY=595;

    SHM_CREATOR();                 /// CREATING SHARED MEMORY SEGMENT
    createActions();
    builder_Menu();            	    /// CREATING MENU from Menu.cpp
    GUI_CREATOR();
    handle_connections();

    imageLabel = new ImgLabel;

    QImage image1("IMG/TT_CHNet_res1.png");
    imageLabel->setPixmap(QPixmap::fromImage(image1));
    imageLabel->setBackgroundRole(QPalette::Base);

    scrollArea->setWidget(imageLabel);
    scrollArea->setAlignment(Qt::AlignCenter);

    timer = new QTimer(this);                                                // TIMER for program control
    connect(timer, SIGNAL(timeout()), this, SLOT(tty_timer()));

    readmultidetcalpar();

    start_thread_tty();
}


void MainWindow::start_thread_tty() {

    tty_ptr = new tty();
    tty_ptr->moveToThread(&test_thread);

    connect(&test_thread, &QThread::finished, tty_ptr, &QObject::deleteLater);
    connect(tty_ptr, &tty::toggle_tab1, this, &MainWindow::toggle_tab1);
    connect(tty_ptr, &tty::toggle_widgets, this, &MainWindow::toggle_widgets);
    connect(tty_ptr, &tty::update_monitor, this, &MainWindow::update_monitor);

    connect(this, &MainWindow::set_target, tty_ptr, &tty::set_target);
    connect(this, &MainWindow::keyence_reading, tty_ptr, &tty::enable_servo);
    connect(this, &MainWindow::start_servo, tty_ptr, &tty::start_servo);

    QSignalMapper *mapper_device_files = new QSignalMapper();
    for (int i = 0; i < 4; i++) {
        mapper_device_files->setMapping(tab1_df_number[i], i);
        connect(tab1_df_number[i], SIGNAL(valueChanged(int)), mapper_device_files, SLOT(map()));
    }   connect(mapper_device_files, SIGNAL(mapped(int)), tty_ptr, SLOT(set_df_minor(int)));

    QSignalMapper *mapper_assign_dfs = new QSignalMapper();
    for (int i = 0; i < 4; i++) {
        mapper_assign_dfs->setMapping(tab1_df_open[i], i);
        connect(tab1_df_open[i], SIGNAL(released()), mapper_assign_dfs, SLOT(map()));
    }   connect(mapper_assign_dfs, SIGNAL(mapped(int)), tty_ptr, SLOT(tty_init(int)));

    QSignalMapper *mapper_init_stages = new QSignalMapper;
    for (int i = 0; i < 3; i++) {
        mapper_init_stages->setMapping(tab1_stage_init[i], i);
        connect(tab1_stage_init[i], SIGNAL(released()), mapper_init_stages, SLOT(map()));
    }   connect(mapper_init_stages, SIGNAL(mapped(int)), tty_ptr, SLOT(stage_init(int)));

    QSignalMapper *mapperMoveStages = new QSignalMapper();
    for (int i = 0; i < 9; i++) {
        mapperMoveStages->setMapping(buttonTab3[i], i);
        connect(buttonTab3[i], SIGNAL(released()), mapperMoveStages, SLOT(map()));
    }   connect(mapperMoveStages, SIGNAL(mapped(int)), tty_ptr, SLOT(move_stage(int)));


    test_thread.start();

};

/* Signals */

void MainWindow::enable_keyence_reading() {
    QCheckBox *tmp = static_cast<QCheckBox*>(this->sender());
    bool active = tmp->checkState();

    servo_checkbox->setEnabled(active);
    emit keyence_reading(active);
}

void MainWindow::enable_servo() {
    QCheckBox *tmp = static_cast<QCheckBox*>(this->sender());
    bool active = tmp->checkState();

    buttonTab3[2]->setEnabled(active);
    buttonTab3[7]->setEnabled(active);
    buttonTab3[8]->setEnabled(active);
    spinboxTab3[2]->setEnabled(active);

    emit start_servo(active);
}


/* Public slots */


void MainWindow::toggle_tab1(bool state) {
    for (int i = 0; i < 3; i++) {
        tab1_df_number[i]->setEnabled(state);
        tab1_df_open[i]->setEnabled(state);
        tab1_stage_init[i]->setEnabled(state);
    }
}

void MainWindow::toggle_widgets(int id) {
    switch (id) {
    case 0: case 1:
        tab3->setEnabled(true);
        break;
    case 2:
        tab2->setEnabled(true);
        break;
    case 3:
        AUTOFOCUS_ON_pushButton->setEnabled(true);
    }
}

void MainWindow::update_monitor(QString message, QString style, int id){
    dev_monitor[id]->setText(message);
    dev_monitor[id]->setStyleSheet(style);
}

void MainWindow::tab3_set_target() {
    QDoubleSpinBox *tmp = static_cast<QDoubleSpinBox*>(this->sender());
    double number = tmp->value();

    for (int id = 0; id < 3; id++) {
        if (tmp == spinboxTab3[id]) emit set_target(id, number);
    }
}


void MainWindow::set_abort_flag() { // raises a flag for abortion
    QMutex m;
    m.lock();
    *(shared_memory_cmd+200) = 1;
    m.unlock();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//                                MAINWINDOW: FURTHER ACTIONS
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void MainWindow::Changeparameters()
{
    bool ok1, ok2, ok3;
    
    int calpar1 = QInputDialog::getInt(this, tr("Multidetector calibration parameters"),tr("Parameter A"), 0, -1000000, 1000000, 0.000001, &ok1);

    int  calpar2 = QInputDialog::getInt(this, tr("Multidetector calibration parameters"),tr("Parameter B"), 0, -1000000, 1000000, 0.000001, &ok2);

    int  scalefactor = QInputDialog::getInt(this, tr("Multidetector calibration parameters"),tr("Scale factor"), 0, 1, 1000000000, 1, &ok3);

    FILE *filecalpar; //name of the file where the channel intervals are specified
    filecalpar = fopen ("../multidetector_calibrationparameters.txt", "w+");
    fprintf(filecalpar, "%d\n", calpar1);
    fprintf(filecalpar, "%d\n", calpar2);
    fprintf(filecalpar, "%d\n", scalefactor);
    fclose(filecalpar);

    *(shared_memory_cmd+101)=calpar1;
    *(shared_memory_cmd+102)=calpar2;
    *(shared_memory_cmd+103)=scalefactor;

}

void MainWindow::readmultidetcalpar() {
    int calpar1 = 0, calpar2 = 0, scalefactor=1;

    FILE *filecalpar;
    filecalpar = fopen ("../multidetector_calibrationparameters.txt", "r");
    if (filecalpar != nullptr) {
        fscanf(filecalpar, "%d", &calpar1);
        fscanf(filecalpar, "%d", &calpar2);
        fscanf(filecalpar, "%d", &scalefactor);
        fclose(filecalpar);
    }

    *(shared_memory_cmd+101) = calpar1;
    *(shared_memory_cmd+102) = calpar2;
    *(shared_memory_cmd+103) = scalefactor;

    if ((calpar1 != 0 || calpar2 != 0) && scalefactor != 0) printf("... Multidetector parameters found\n");
}

void MainWindow::hideImage() {
    if (MapIsOpened) {
        //QImage startimage("IMG/TT_CHNet_extended_395_395_3.png");
        //imageLabel->setPixmap(QPixmap::fromImage(startimage));
        MapIsOpened=false;
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//                                MAINWINDOW MAIN COMMAND CYCLE --> TIMER_EVENT
//                                
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void MainWindow::tty_timer() {

    if (XYscanning)ScanXY();
    CheckSegFault();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//                                MOTOR SETTINGS
//                                
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void MainWindow::scan_parameters(double val) {
    QDoubleSpinBox *temp = static_cast<QDoubleSpinBox*>(this->sender());
    int i = 0;

    //Px=val*1000 (and Py)
    //Xmin1=val*1000 (and Xmax1, Ymin1, Ymax1)
    //pixel_Xstep=val (and pixel_Ystep)
    //positionX=val (and positionY)

    *(shared_memory_cmd+300+i) = val*1000;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//                                DAQ SETTINGS AND DIGITISER PROGRAM CONTROL
//                                
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void MainWindow::USB_DAQ()                                  // DAQ VIA USB
{
    {DAQ_TYPE=1; qDebug()<<"USB link active..."<<"DAQ_TYPE: "<<DAQ_TYPE;}
}

void MainWindow::OPTICAL_DAQ()                              // DAQ VIA OPTICAL LINK
{
    {DAQ_TYPE=0; qDebug()<<"OPTICAL link active..."<<"DAQ_TYPE: "<<DAQ_TYPE;}
}

void MainWindow::set_PMAcquisitionTime() {
    bool dlgok;
    measuring_time = QInputDialog::getInt(this, tr("Set Acquisition Time"), tr("Seconds:"),300, 0, 18000, 60, &dlgok);
    if (dlgok) {
        printf("... Point-mode acquisition time set to:\t%d seconds\n", measuring_time);
    }
}

void MainWindow::SelDigiCh0()
{

    if(DigitizerChannel0->isChecked())
    {

        *(shared_memory_cmd+100)=0;
        DigitizerChannel1->setChecked(false); DigitizerChannel0and1->setChecked(false);
    }

}

void MainWindow::SelDigiCh1()
{

    if(DigitizerChannel1->isChecked())
    {
        *(shared_memory_cmd+100)=1;
        DigitizerChannel0->setChecked(false); DigitizerChannel0and1->setChecked(false);
    }

}

void MainWindow::SelDigiCh0and1()
{

    if(DigitizerChannel0and1->isChecked())
    {
        *(shared_memory_cmd+100)=2;
        DigitizerChannel0->setChecked(false); DigitizerChannel1->setChecked(false);
    }

}

void MainWindow::CheckSegFault()                           // DAQ: MEMORY CONTROL FOR SEGMENTATION FAULT
{
    if(*(shared_memory2+6)==1)
    {
        if(*(shared_memory_cmd+70)==1)
        {
            int pidVme=*(shared_memory_cmd+80);
            sprintf(process, "kill -s TERM %i &", pidVme);
            system(process);
            *(shared_memory_cmd+70)=0;
            SaveTxt();
            qDebug()<<"[!] Acquisition interrumpted because shared memory is full";
        }

        if(XYscanning==true)
        {
            XYscanning=false;
            //abort
            qDebug()<<"[!] Scan interrumpted because shared memory is full";
        }
        *(shared_memory2+6)=0;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//                                MAP - PIXEL AND SPECTRUM MANAGEMENT
//                                
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void MainWindow::open_MAP()                                      // MAP: OPEN MAP
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open File"), QDir::currentPath());
    if (!fileName.isEmpty())
    {
        QImage image(fileName);
        if (image.isNull())
        {
            QMessageBox::information(this, tr("Image Viewer"),
                                     tr("Cannot load %1.").arg(fileName));
            return;
        }
        imageLabel->setPixmap(QPixmap::fromImage(image));
        scaleFactor = 1.0;
    }
}

void MainWindow::SelectChannels()                             // MAP: CHANNEL SELECTION
{
    bool ok1, ok2;
    if(*(shared_memory+24)==0)
    {
        double chan1 = QInputDialog::getInt(this, tr("Lower Channel"),tr("ChLow:"), 0, 0, 16384, 1, &ok1);
        if (ok1)
        {qDebug()<<"New lower channel "<<chan1<<'\n'; ChMin=chan1;}
        double  chan2 = QInputDialog::getInt(this, tr("Upper Channel"),tr("ChHigh:"), 16384, 0, 16384, 1, &ok2);
        if (ok2)
        {qDebug()<<"New upper channel "<<chan2<<'\n'; ChMax=chan2;}
    }
    else if (*(shared_memory+24) == 1) {
            double chan1 = QInputDialog::getDouble(this, tr("Lower Energy"), tr("ELow:"), 0, 0, 60, 2, &ok1);
            if (ok1)
            {qDebug()<<"New lower energy "<<chan1<<'\n'; ChMin=int(chan1*1000);energychanged=true;}
            double chan2 = QInputDialog::getDouble(this, tr("Upper Energy"), tr("EHigh:"), 60, 0, 60, 2, &ok2);
            if (ok2)
            {qDebug()<<"New upper energy "<<chan2<<'\n'; ChMax=int(chan2*1000);energychanged=true;}
        }
}




void MainWindow::Pixels() { // Change pixel dimension
    bool ok1;
    int px = QInputDialog::getInt(this, "Set Pixel Size", "Pixel side dimension:", 1, 1, 30, 1, &ok1);

    if (ok1) {
        Pixeldim = px;
        printf("[!] New pixel dimension: %d", px);
    }
    else printf("[!] Couldn't obtain new pixel dimensions\n");
}

void MainWindow::LoadNewFile_SHM() { // Loads a new file in memory
    LoadNewFileWithNoCorrection_SHM();
    displayImage_SHM();
}

void MainWindow::LoadElementsMapSum() {

    ChMin1 = 0; ChMin2 = 0; ChMin3 = 0;
    ChMax1 = 0; ChMax2 = 0; ChMax3 = 0;

    elementsdlg = new QDialog;
    elementsdlg->setFixedSize(400, 200);

    QLabel *labelElement0 = new QLabel("<b>Choose limits for elements shown in,<\b>");
    QLabel *labelElement1 = new QLabel("<b>red:<\b>");
    QLabel *labelElement2 = new QLabel("<b>green:<\b>");
    QLabel *labelElement3 = new QLabel("<b>blue:<\b>");

    QSpinBox *spinboxArray[6];
    QDoubleSpinBox *dspinboxArray[6];

    if (*(shared_memory+24)) {
        QSignalMapper *mapperElementSum = new QSignalMapper();
        for (int i = 0; i < 6; i++) {
            dspinboxArray[i] = new QDoubleSpinBox(this);
            dspinboxArray[i]->setMinimum(0.00);
            dspinboxArray[i]->setMaximum(60.00);
            dspinboxArray[i]->setSingleStep(1.0);
            if ((i % 2) == 0) dspinboxArray[i]->setPrefix("Min: ");
            else dspinboxArray[i]->setPrefix("Max: ");

            mapperElementSum->setMapping(dspinboxArray[i], i);
            connect(dspinboxArray[i], SIGNAL(valueChanged(double)), mapperElementSum, SLOT(map()));
        }
        connect(mapperElementSum, SIGNAL(mapped(int)), this, SLOT(writeCompMapLimits(int)));
    }

    else {
        QSignalMapper *mapperElementSum = new QSignalMapper();
        for (int i = 0; i < 6; i++) {
            spinboxArray[i] = new QSpinBox(this);
            spinboxArray[i]->setMinimum(0);
            spinboxArray[i]->setMaximum(16384);
            spinboxArray[i]->setSingleStep(1);
            if ((i % 2) == 0) spinboxArray[i]->setPrefix("Min: ");
            else spinboxArray[i]->setPrefix("Max: ");

            mapperElementSum->setMapping(spinboxArray[i], i);
            connect(spinboxArray[i], SIGNAL(valueChanged(int)), mapperElementSum, SLOT(map()));
        }
        connect(mapperElementSum, SIGNAL(mapped(int)), this, SLOT(writeCompMapLimits(int)));
    }

    QDialogButtonBox *buttonBox = new QDialogButtonBox(Qt::Horizontal);

    OKbutton = new QPushButton("Ok");
    CANCELbutton = new QPushButton("Cancel");
    buttonBox->addButton(OKbutton, QDialogButtonBox::AcceptRole);
    buttonBox->addButton(CANCELbutton, QDialogButtonBox::AcceptRole);

    connect(OKbutton, SIGNAL(clicked()), elementsdlg, SLOT(close()));
    connect(OKbutton, SIGNAL(clicked()), this, SLOT(LoadSHM_SumMap()));
    connect(CANCELbutton, SIGNAL(clicked()), elementsdlg, SLOT(close()));


    QGridLayout *Layout = new QGridLayout(elementsdlg);
    Layout->setSpacing(5);
    Layout->addWidget(labelElement0, 0, 0, 1, 3);
    Layout->addWidget(labelElement1, 1, 0);
    Layout->addWidget(labelElement2, 2, 0);
    Layout->addWidget(labelElement3, 3, 0);
    if (*(shared_memory+24)) {
        Layout->addWidget(dspinboxArray[0], 1, 1);
        Layout->addWidget(dspinboxArray[1], 1, 2);
        Layout->addWidget(dspinboxArray[2], 2, 1);
        Layout->addWidget(dspinboxArray[3], 2, 2);
        Layout->addWidget(dspinboxArray[4], 3, 1);
        Layout->addWidget(dspinboxArray[5], 3, 2);
    }
    else {
        Layout->addWidget(spinboxArray[0], 1, 1);
        Layout->addWidget(spinboxArray[1], 1, 2);
        Layout->addWidget(spinboxArray[2], 2, 1);
        Layout->addWidget(spinboxArray[3], 2, 2);
        Layout->addWidget(spinboxArray[4], 3, 1);
        Layout->addWidget(spinboxArray[5], 3, 2);
    }
    Layout->addWidget(buttonBox, 4, 0, 1, 3, Qt::AlignRight);

    elementsdlg->show();

}


void MainWindow::writeCompMapLimits(int id) {
    QSignalMapper *temp = static_cast<QSignalMapper*>(this->sender());
    if (*(shared_memory+24)) {
        QDoubleSpinBox *spnbox = static_cast<QDoubleSpinBox*>(temp->mapping(id));
        double value = spnbox->value();

        if (id == 0) ChMin1 = value;
        else if (id == 1) ChMax1 = value;
        else if (id == 2) ChMin2 = value;
        else if (id == 3) ChMax2 = value;
        else if (id == 4) ChMin3 = value;
        else if (id == 5) ChMax3 = value;
        else printf("[!] Unknown sender to the composed map limits function");
    }
    else {
        QSpinBox *spnbox = static_cast<QSpinBox*>(temp->mapping(id));
        int value = spnbox->value();

        if (id == 0) ChMin1 = static_cast<double>(value);
        else if (id == 1) ChMax1 = static_cast<double>(value);
        else if (id == 2) ChMin2 = static_cast<double>(value);
        else if (id == 3) ChMax2 = static_cast<double>(value);
        else if (id == 4) ChMin3 = static_cast<double>(value);
        else if (id == 5) ChMax3 = static_cast<double>(value);
        else printf("[!] Unknown sender to the composed map limits function");
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//                                FILES MANAGEMENT
//                                
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void MainWindow::LoadTxt()  { // Writes values of binary file into shared memory
    QString loadDir = "/home/frao/Desktop/XRFData";
    QString text = QFileDialog::getOpenFileName(this, "Open file..", loadDir);

    if (!text.isEmpty()) {
        QFile file(text);
        if (file.exists()) {
            file.open(QIODevice::ReadOnly);
            QString line = file.readLine();

            int i = 0;
            if (line.startsWith("v")) { // Data with new header protocol
                line = file.readLine();
                *(shared_memory_cmd+50) = line.toInt(); // X start
                line = file.readLine();
                *(shared_memory_cmd+51) = line.toInt(); // X end
                line = file.readLine();
                *(shared_memory_cmd+52) = line.toInt(); // Y start
                line = file.readLine();
                *(shared_memory_cmd+53) = line.toInt(); // Y end
                line = file.readLine();
                *(shared_memory_cmd+60) = line.toInt(); // X step
                line = file.readLine();
                *(shared_memory_cmd+61) = line.toInt(); // Y step
                line = file.readLine();
                *(shared_memory_cmd+67) = line.toInt(); // Velocity
            }
            else {
                *(shared_memory_cmd+60) = QInputDialog::getInt(this, "Set X pixel step", "X step (um):", 500, 1, 2000, 1, nullptr);
                *(shared_memory_cmd+61) = QInputDialog::getInt(this, "Set Y pixel step", "Y step (um):", 500, 1, 2000, 1, nullptr);

                *(shared_memory2+10+(++i)) = line.toInt();
            }

            while (!file.atEnd()) {
                line = file.readLine();
                *(shared_memory2+10+(++i)) = line.toInt();
                if (i == 1) printf("[!] First position: %d\n", *(shared_memory2+10+i));
            }
            file.close();
            *(shared_memory2+4) = i;
        }
        else printf("[!] File not found\n");
    }
    LoadNewFile_SHM();
}




void MainWindow::SaveTxt() {
    QString saveDir = "/home/frao/Desktop/XRFData";
    QString percorso = QFileDialog::getSaveFileName(this,tr("Save as"), saveDir);

    QFile file2(percorso);
    file2.open(QIODevice::ReadWrite);
    QTextStream out2(&file2);
    int Ntot = *(shared_memory2+4);
    out2<<"ver.001"<<'\n';
    out2<<*(shared_memory_cmd+50)<<'\n';//writes Xmin
    out2<<*(shared_memory_cmd+51)<<'\n';//writes Xmax
    out2<<*(shared_memory_cmd+52)<<'\n';//writes Ymin
    out2<<*(shared_memory_cmd+53)<<'\n';//writes Ymax
    out2<<*(shared_memory_cmd+60)<<'\n';//writes Xstep
    out2<<*(shared_memory_cmd+61)<<'\n';//writes Ystep
    out2<<*(shared_memory_cmd+67)<<'\n';//writes the scan velocity
    for (int i = 1; i <= Ntot; i++) {
        out2<<*(shared_memory2+10+i)<<'\n';
    }
    file2.close();


    string str = percorso.toStdString();
    char *cstr = &str[0u];
    printf("[!] File saved in: %s", cstr);
}




void MainWindow::closeEvent(QCloseEvent *event) {
    event->ignore();
    event->accept();
}

MainWindow::~MainWindow() {

    int processIDs[7][2] = { { 0 }, { 0 } };

    printf("\n... Terminating data acquisition session\n");
    if (*(shared_memory_cmd+70)) *(shared_memory_cmd+70) = 0;

    for (int i = 0; i < 7; i++) {
        processIDs[i][0] = *(shared_memory_cmd+i+71);
        processIDs[i][1] = *(shared_memory_cmd+i+81);
    }

    printf("... Dettaching shared memory segments\n");
    for (int i = 0; i < 8; i++) if (shmid[i] != -1) shmctl(shmid[i], IPC_RMID, 0);
    shmdt(shared_memory);
    shmdt(shared_memory2);
    shmdt(shared_memory3);
    shmdt(shared_memory4);
    shmdt(shared_memory5);
    shmdt(shared_memory_rate);
    shmdt(shared_memory_cmd);

    qDebug()<<"... Killing child processes";
    for (int i = 0; i < 7; i++) {
        if (processIDs[i][0] == 1) {
            qDebug()<<"... Killing process with ID: "<<processIDs[i][1];
            sprintf(process, "kill -s TERM %i &", processIDs[i][1]);
            system(process);
        }
    }

    test_thread.quit();
    test_thread.wait();

}

void MainWindow::Info1_1() {
    system("evince manual/Info_software_general.pdf &");
}

void MainWindow::Info1_2() {
    system("evince manual/Info_shared_memory.pdf &");
}

void MainWindow::Info2_1() {
    system("evince manual/Info_kernel_modules.pdf &");
}

