#include "canvas.h"
#include <QPushButton>
#include <QLayout>
#include <QTimer>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QMessageBox>

#include <stdlib.h>

#include <TCanvas.h>
#include <TVirtualX.h>
#include <TSystem.h>
#include <TFormula.h>
#include <TF1.h>
#include <TH1.h>
#include <TFrame.h>
#include <TTimer.h>
#include "canvas.h"

#include "Riostream.h"


//------------------------------------------------------------------------------

//______________________________________________________________________________
QRootCanvas::QRootCanvas(QWidget *parent) : QWidget(parent, 0), fCanvas(0)
{
   // QRootCanvas constructor.

   // set options needed to properly update the canvas when resizing the widget
   // and to properly handle context menus and mouse move events
   setAttribute(Qt::WA_PaintOnScreen, true);
   setAttribute(Qt::WA_OpaquePaintEvent, true);
   setMinimumSize(300, 200);
   setUpdatesEnabled(kFALSE);
   setMouseTracking(kTRUE);

   // register the QWidget in TVirtualX, giving its native window id
   int wid = gVirtualX->AddWindow((ULong_t)winId(), 600, 400);
   // create the ROOT TCanvas, giving as argument the QWidget registered id
   fCanvas = new TCanvas("Root Canvas", width(), height(), wid);
}

//______________________________________________________________________________
void QRootCanvas::mouseMoveEvent(QMouseEvent *e)
{
   // Handle mouse move events.

   if (fCanvas) {
      if (e->buttons() & Qt::LeftButton) {
         fCanvas->HandleInput(kButton1Motion, e->x(), e->y());
      } else if (e->buttons() & Qt::MidButton) {
         fCanvas->HandleInput(kButton2Motion, e->x(), e->y());
      } else if (e->buttons() & Qt::RightButton) {
         fCanvas->HandleInput(kButton3Motion, e->x(), e->y());
      } else {
         fCanvas->HandleInput(kMouseMotion, e->x(), e->y());
      }
   }
}

//______________________________________________________________________________
void QRootCanvas::mousePressEvent( QMouseEvent *e )
{
   // Handle mouse button press events.

   if (fCanvas) {
      switch (e->button()) {
         case Qt::LeftButton :
            fCanvas->HandleInput(kButton1Down, e->x(), e->y());
            break;
         case Qt::MidButton :
            fCanvas->HandleInput(kButton2Down, e->x(), e->y());
            break;
         case Qt::RightButton :
            // does not work properly on Linux...
            // ...adding setAttribute(Qt::WA_PaintOnScreen, true) 
            // seems to cure the problem
            fCanvas->HandleInput(kButton3Down, e->x(), e->y());
            break;
         default:
            break;
      }
   }
}

//______________________________________________________________________________
void QRootCanvas::mouseReleaseEvent( QMouseEvent *e )
{
   // Handle mouse button release events.

   if (fCanvas) {
      switch (e->button()) {
         case Qt::LeftButton :
            fCanvas->HandleInput(kButton1Up, e->x(), e->y());
            break;
         case Qt::MidButton :
            fCanvas->HandleInput(kButton2Up, e->x(), e->y());
            break;
         case Qt::RightButton :
            // does not work properly on Linux...
            // ...adding setAttribute(Qt::WA_PaintOnScreen, true) 
            // seems to cure the problem
            fCanvas->HandleInput(kButton3Up, e->x(), e->y());
            break;
         default:
            break;
      }
   }
}

//______________________________________________________________________________
void QRootCanvas::resizeEvent( QResizeEvent * )
{
   // Handle resize events.

   if (fCanvas) {
      fCanvas->Resize();
      fCanvas->Update();
   }
}

//______________________________________________________________________________
void QRootCanvas::paintEvent( QPaintEvent * )
{
   // Handle paint events.

   if (fCanvas) {
      fCanvas->Resize();
      fCanvas->Update();
   }
}

//------------------------------------------------------------------------------

//______________________________________________________________________________
QMainCanvas::QMainCanvas(QWidget *parent) : QWidget(parent)
{
   // QMainCanvas constructor.

   QVBoxLayout *l = new QVBoxLayout(this);
   l->addWidget(canvas = new QRootCanvas(this));
   l->addWidget(b = new QPushButton("&Open File", this));
   connect(b, SIGNAL(clicked()), this, SLOT(clicked1()));
   fRootTimer = new QTimer( this );
   QObject::connect( fRootTimer, SIGNAL(timeout()), this, SLOT(handle_root_events()) );
   fRootTimer->start( 20 );
}

//______________________________________________________________________________
void QMainCanvas::clicked1()
{
   // Handle the "Draw Histogram" button clicked() event.



/*
   static TH1F *h1f = 0;
   new TFormula("form1","abs(sin(x)/x)");
   TF1 *sqroot = new TF1("sqroot","x*gaus(0) + [3]*form1", 0, 10);
   sqroot->SetParameters(10, 4, 1, 20);

   // Create a one dimensional histogram (one float per bin)
   // and fill it following the distribution in function sqroot.
   canvas->getCanvas()->Clear();
   canvas->getCanvas()->cd();
   canvas->getCanvas()->SetBorderMode(0);
   canvas->getCanvas()->SetFillColor(0);
   canvas->getCanvas()->SetGrid();
   if (h1f) delete h1f;
   h1f = new TH1F("h1f","Test random numbers", 200, 0, 10);
   h1f->SetFillColor(kViolet + 2);
   h1f->SetFillStyle(3001);
   h1f->FillRandom("sqroot", 10000);
   h1f->Draw(); 
*/
string input_ver;      
Int_t npoint=0;         Int_t npoint_wb=0;    Int_t input;
Double_t bincontent=0;  Double_t bincontent_int=0;
bool firstbin=true;

TH1F *h0=new TH1F("h0","XRF: pixel content",16384,0,16383);
TGraph2D *dt_xrf = new TGraph2D();


// HEADER

int Xmin;  // Map(Xmin)
int Xmax;  // Map(Xmax)
int Ymin;  // Map(Ymin)
int Ymax;  // Map(Ymax)
int StepX; // Map(StepX)
int StepY; // Map(StepY)
int Speed; // Map(Speed)

// defining array for 3D plot
int Xbin=(Xmax-Xmin)/StepX;
int Ybin=(Ymax-Ymin)/StepY;
int ix=0, iy=0;
Double_t Xcoord; Double_t Xcoord_M_wb; Double_t Xcoord_m_wb; 
Double_t Ycoord; 


  QString text = QFileDialog::getOpenFileName(this,tr("Open Spectrum"), QDir::currentPath());
   if (!text.isEmpty())
    {
     QFile file(text);
     if(file.exists())
      {
       file.open(QIODevice::ReadOnly);
       QString line;
       line = file.readLine(); qDebug()<<line;
       //printf("Release: %c\n",line);
       line = file.readLine(); Xmin=line.toInt();  printf("Xmin: %d\n",Xmin);   // Map(Xmin)
       line = file.readLine(); Xmax=line.toInt();  printf("Xmax: %d\n",Xmax);   // Map(Xmax)
       line = file.readLine(); Ymin=line.toInt();  printf("Ymin: %d\n",Ymin);   // Map(Ymin)
       line = file.readLine(); Ymax=line.toInt();  printf("Ymax: %d\n",Ymax);   // Map(Ymax)
       line = file.readLine(); StepX=line.toInt(); printf("StepX: %d\n",StepX); // Map(StepX)
       line = file.readLine(); StepY=line.toInt(); printf("StepY: %d\n",StepY); // Map(StepY)
       line = file.readLine(); Speed=line.toInt(); printf("Speed: %d\n",Speed); // Map(Speed)

       Xbin=(Xmax-Xmin)/StepX;
       Ybin=(Ymax-Ymin)/StepY;
       Xcoord_M_wb=(Xmax/StepX);
       Xcoord_m_wb=(Xmin/StepX); 


       while( !file.atEnd() ) 
         {
          line = file.readLine(); input=line.toInt();
          if(input > 100000 && firstbin)
           { 
            Xcoord=(input-50000000)/StepX;
            line = file.readLine(); input=line.toInt(); 
            Ycoord=(input-60000000)/StepY;
            printf("first_done\n");
           }
          if(input > 100000 && !firstbin)
           { 
            dt_xrf->SetPoint(npoint,Xcoord,Ycoord,bincontent); 
            Xcoord=(input-50000000)/StepX; 
            line = file.readLine(); input=line.toInt(); 
            Ycoord=(input-60000000)/StepY;
            h0->Fill(bincontent,1);
            bincontent=0;
            npoint++;
            }
          if (input < 10000)
           {    
            bincontent=bincontent+1;      
           }
         firstbin=false; 

      }
     dt_xrf->SetPoint(npoint,Xcoord,Ycoord,bincontent);
 }
     file.close();

 }
//    TCanvas *c0=new TCanvas("c0","CHNet - 2D",50,50,800,700);
      h0->GetXaxis()->SetTitle("Channel");
      h0->GetYaxis()->SetTitle("Counts");
//    h0->Draw();

      h0->Fit("gaus");

//    TCanvas *c1=new TCanvas("c1","CHNet - 2D",50,50,800,700);
//      h1->GetXaxis()->SetTitle("Channel");
//      h1->GetYaxis()->SetTitle("Counts");
//    h1->Draw();

//    TCanvas *c2 = new TCanvas("c2","CHNet - 3D",0,0,1200,700);
      dt_xrf->SetTitle("XRF Map: pixel content");
//      gStyle->SetPalette(55);
//    dt_xrf->Draw("surf1");


   canvas->getCanvas()->Clear();
   canvas->getCanvas()->cd();
   canvas->getCanvas()->SetBorderMode(0);
   canvas->getCanvas()->SetFillColor(0);
   canvas->getCanvas()->SetGrid();


//      TCanvas *c4 = new TCanvas("c4","CHNet - 3D",0,0,1800,1200);   
//      c4->Divide(2,2);
//      c4->cd(1);
//      h0->Draw();
//      c4->cd(2);
      dt_xrf->Draw("surf1");
//      c4->cd(3);
//      h1->Draw();
//      c4->cd(4);
//      dt_xrf_wb->Draw("surf1");









   canvas->getCanvas()->Modified();
   canvas->getCanvas()->Update();

}

//______________________________________________________________________________
void QMainCanvas::handle_root_events()
{
   //call the inner loop of ROOT
   gSystem->ProcessEvents();
}

//______________________________________________________________________________
void QMainCanvas::changeEvent(QEvent * e)
{
   if (e->type() == QEvent ::WindowStateChange) {
      QWindowStateChangeEvent * event = static_cast< QWindowStateChangeEvent * >( e );
      if (( event->oldState() & Qt::WindowMaximized ) ||
          ( event->oldState() & Qt::WindowMinimized ) ||
          ( event->oldState() == Qt::WindowNoState && 
            this->windowState() == Qt::WindowMaximized )) {
         if (canvas->getCanvas()) {
            canvas->getCanvas()->Resize();
            canvas->getCanvas()->Update();
         }
      }
   }
}