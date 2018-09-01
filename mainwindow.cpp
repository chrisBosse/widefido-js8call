﻿//---------------------------------------------------------- MainWindow
#include "mainwindow.h"
#include <cmath>
#include <cinttypes>
#include <limits>
#include <functional>
#include <fstream>
#include <iterator>
#include <fftw3.h>
#include <QLineEdit>
#include <QRegExpValidator>
#include <QRegExp>
#include <QRegularExpression>
#include <QDesktopServices>
#include <QUrl>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QtConcurrent/QtConcurrentRun>
#include <QProgressDialog>
#include <QHostInfo>
#include <QVector>
#include <QCursor>
#include <QToolTip>
#include <QAction>
#include <QActionGroup>
#include <QSplashScreen>

#include "APRSISClient.h"
#include "revision_utils.hpp"
#include "qt_helpers.hpp"
#include "NetworkAccessManager.hpp"
#include "soundout.h"
#include "soundin.h"
#include "Modulator.hpp"
#include "Detector.hpp"
#include "plotter.h"
#include "echoplot.h"
#include "echograph.h"
#include "fastplot.h"
#include "fastgraph.h"
#include "about.h"
#include "messageaveraging.h"
#include "widegraph.h"
#include "sleep.h"
#include "logqso.h"
#include "decodedtext.h"
#include "Radio.hpp"
#include "Bands.hpp"
#include "TransceiverFactory.hpp"
#include "StationList.hpp"
#include "LiveFrequencyValidator.hpp"
#include "MessageClient.hpp"
#include "wsprnet.h"
#include "signalmeter.h"
#include "HelpTextWindow.hpp"
#include "SampleDownloader.hpp"
#include "Audio/BWFFile.hpp"
#include "MultiSettings.hpp"
#include "MaidenheadLocatorValidator.hpp"
#include "CallsignValidator.hpp"
#include "EqualizationToolsDialog.hpp"
#include "SelfDestructMessageBox.h"

#include "ui_mainwindow.h"
#include "moc_mainwindow.cpp"


extern "C" {
  //----------------------------------------------------- C and Fortran routines
  void symspec_(struct dec_data *, int* k, int* ntrperiod, int* nsps, int* ingain,
                int* minw, float* px, float s[], float* df3, int* nhsym, int* npts8,
                float *m_pxmax);

  void hspec_(short int d2[], int* k, int* nutc0, int* ntrperiod, int* nrxfreq, int* ntol,
              bool* bmsk144, bool* bcontest, bool* btrain, double const pcoeffs[], int* ingain,
              char mycall[], char hiscall[], bool* bshmsg, bool* bswl, char ddir[], float green[],
              float s[], int* jh, float *pxmax, float *rmsNoGain, char line[], char mygrid[],
              fortran_charlen_t, fortran_charlen_t, fortran_charlen_t, fortran_charlen_t,
              fortran_charlen_t);
//  float s[], int* jh, char line[], char mygrid[],

  void genft8_(char* msg, char* MyGrid, bool* bcontest, int* i3bit, char* msgsent,
               char ft8msgbits[], int itone[], fortran_charlen_t, fortran_charlen_t,
               fortran_charlen_t);

  void gen4_(char* msg, int* ichk, char* msgsent, int itone[],
               int* itext, fortran_charlen_t, fortran_charlen_t);

  void gen9_(char* msg, int* ichk, char* msgsent, int itone[],
               int* itext, fortran_charlen_t, fortran_charlen_t);

  void genmsk144_(char* msg, char* MyGrid, int* ichk, bool* bcontest,
                  char* msgsent, int itone[], int* itext, fortran_charlen_t,
                  fortran_charlen_t, fortran_charlen_t);

  void gen65_(char* msg, int* ichk, char* msgsent, int itone[],
              int* itext, fortran_charlen_t, fortran_charlen_t);

  void genqra64_(char* msg, int* ichk, char* msgsent, int itone[],
              int* itext, fortran_charlen_t, fortran_charlen_t);

  void genwspr_(char* msg, char* msgsent, int itone[], fortran_charlen_t, fortran_charlen_t);

  void genwspr_fsk8_(char* msg, char* msgsent, int itone[], fortran_charlen_t, fortran_charlen_t);

  void geniscat_(char* msg, char* msgsent, int itone[], fortran_charlen_t, fortran_charlen_t);

  void azdist_(char* MyGrid, char* HisGrid, double* utch, int* nAz, int* nEl,
               int* nDmiles, int* nDkm, int* nHotAz, int* nHotABetter,
               fortran_charlen_t, fortran_charlen_t);

  void morse_(char* msg, int* icw, int* ncw, fortran_charlen_t);

  int ptt_(int nport, int ntx, int* iptt, int* nopen);

  void wspr_downsample_(short int d2[], int* k);

  int savec2_(char* fname, int* TR_seconds, double* dial_freq, fortran_charlen_t);

  void avecho_( short id2[], int* dop, int* nfrit, int* nqual, float* f1,
                float* level, float* sigdb, float* snr, float* dfreq,
                float* width);

  void fast_decode_(short id2[], int narg[], int* ntrperiod,
                    char msg[], char mycall[], char hiscall[],
                    fortran_charlen_t, fortran_charlen_t, fortran_charlen_t);
  void degrade_snr_(short d2[], int* n, float* db, float* bandwidth);

  void wav12_(short d2[], short d1[], int* nbytes, short* nbitsam2);

  void refspectrum_(short int d2[], bool* bclearrefspec,
                    bool* brefspec, bool* buseref, const char* c_fname, fortran_charlen_t);

  void freqcal_(short d2[], int* k, int* nkhz,int* noffset, int* ntol,
                char line[], fortran_charlen_t);

  void fix_contest_msg_(char* MyGrid, char* msg, fortran_charlen_t, fortran_charlen_t);

  void calibrate_(char data_dir[], int* iz, double* a, double* b, double* rms,
                  double* sigmaa, double* sigmab, int* irc, fortran_charlen_t);

  void foxgen_();

  void plotsave_(float swide[], int* m_w , int* m_h1, int* irow);
}

int volatile itone[NUM_ISCAT_SYMBOLS];  //Audio tones for all Tx symbols
int volatile icw[NUM_CW_SYMBOLS];       //Dits for CW ID
struct dec_data dec_data;               // for sharing with Fortran

int outBufSize;
int rc;
qint32  g_iptt {0};
wchar_t buffer[256];
float fast_green[703];
float fast_green2[703];
float fast_s[44992];                                    //44992=64*703
float fast_s2[44992];
int   fast_jh {0};
int   fast_jhpeak {0};
int   fast_jh2 {0};
int narg[15];
QVector<QColor> g_ColorTbl;

namespace
{
  Radio::Frequency constexpr default_frequency {14078500};

  QRegExp message_alphabet {"[^\\x00-\\x1F]*"}; // base alphabet supported by FT8CALL

  // grid exact match excluding RR73
  QRegularExpression grid_regexp {"\\A(?![Rr]{2}73)[A-Ra-r]{2}[0-9]{2}([A-Xa-x]{2}){0,1}\\z"};

  bool message_is_73 (int type, QStringList const& msg_parts)
  {
    return type >= 0
      && (((type < 6 || 7 == type)
           && (msg_parts.contains ("73") || msg_parts.contains ("RR73")))
          || (type == 6 && !msg_parts.filter ("73").isEmpty ()));
  }

  int ms_minute_error ()
  {
    auto const& now = QDateTime::currentDateTime ();
    auto const& time = now.time ();
    auto second = time.second ();
    return now.msecsTo (now.addSecs (second > 30 ? 60 - second : -second)) - time.msec ();
  }

  QString since(QDateTime time){
      int delta = time.toUTC().secsTo(QDateTime::currentDateTimeUtc());
      if(delta < 30){
          return QString("now");
      }

      int seconds = delta % 60;
      delta = delta / 60;
      int minutes = delta % 60;
      delta = delta / 60;
      int hours = delta % 24;
      delta = delta / 24;
      int days = delta;

      if(days){
          return QString("%1d").arg(days);
      }
      if(hours){
          return QString("%1h").arg(hours);
      }
      if(minutes){
          return QString("%1m").arg(minutes);
      }
      if(seconds){
          return QString("%1s").arg(seconds - seconds%15);
      }

      return QString {};
  }

  void clearTableWidget(QTableWidget *widget){
      if(!widget){
          return;
      }
      for(int i = widget->rowCount(); i >= 0; i--){
        widget->removeRow(i);
      }
  }

#if 0
  int round(int numToRound, int multiple)
  {
   if(multiple == 0)
   {
    return numToRound;
   }

   int roundDown = ( (int) (numToRound) / multiple) * multiple;

   if(numToRound - roundDown > multiple/2){
    return roundDown + multiple;
   }

   return roundDown;
  }
#endif

  int roundUp(int numToRound, int multiple)
  {
   if(multiple == 0)
   {
    return numToRound;
   }

   int roundDown = ( (int) (numToRound) / multiple) * multiple;
   return roundDown + multiple;
  }

  QString rstrip(const QString& str) {
    int n = str.size() - 1;
    for (; n >= 0; --n) {
      if (str.at(n).isSpace()) {
          continue;
      }
      return str.left(n + 1);
    }
    return "";
  }

  QString lstrip(const QString& str) {
    int len = str.size();
    for (int n = 0; n < len; n++) {
        if(str.at(n).isSpace()){
            continue;
        }
        return str.mid(n);
    }
    return "";
  }
}

//--------------------------------------------------- MainWindow constructor
MainWindow::MainWindow(QDir const& temp_directory, bool multiple,
                       MultiSettings * multi_settings, QSharedMemory *shdmem,
                       unsigned downSampleFactor,
                       QSplashScreen * splash, QWidget *parent) :
  QMainWindow(parent),
  m_network_manager {this},
  m_valid {true},
  m_splash {splash},
  m_revision {revision ()},
  m_multiple {multiple},
  m_multi_settings {multi_settings},
  m_configurations_button {0},
  m_settings {multi_settings->settings ()},
  ui(new Ui::MainWindow),
  m_config {temp_directory, m_settings, this},
  m_WSPR_band_hopping {m_settings, &m_config, this},
  m_WSPR_tx_next {false},
  m_rigErrorMessageBox {MessageBox::Critical, tr ("Rig Control Error")
      , MessageBox::Cancel | MessageBox::Ok | MessageBox::Retry},
  m_wideGraph (new WideGraph(m_settings)),
  m_echoGraph (new EchoGraph(m_settings)),
  m_fastGraph (new FastGraph(m_settings)),
  // no parent so that it has a taskbar icon
  m_logDlg (new LogQSO (program_title (), m_settings, &m_config, nullptr)),
  m_lastDialFreq {0},
  m_dialFreqRxWSPR {0},
  m_detector {new Detector {RX_SAMPLE_RATE, NTMAX, downSampleFactor}},
  m_FFTSize {6192 / 2},         // conservative value to avoid buffer overruns
  m_soundInput {new SoundInput},
  m_modulator {new Modulator {TX_SAMPLE_RATE, NTMAX}},
  m_soundOutput {new SoundOutput},
  m_msErase {0},
  m_secBandChanged {0},
  m_freqNominal {0},
  m_freqTxNominal {0},
  m_s6 {0.},
  m_tRemaining {0.},
  m_DTtol {3.0},
  m_waterfallAvg {1},
  m_ntx {1},
  m_gen_message_is_cq {false},
  m_send_RR73 {false},
  m_XIT {0},
  m_sec0 {-1},
  m_RxLog {1},      //Write Date and Time to RxLog
  m_nutc0 {999999},
  m_ntr {0},
  m_tx {0},
  m_TRperiod {60},
  m_inGain {0},
  m_secID {0},
  m_idleMinutes {0},
  m_nSubMode {0},
  m_nclearave {1},
  m_pctx {0},
  m_nseq {0},
  m_nWSPRdecodes {0},
  m_k0 {9999999},
  m_nPick {0},
  m_frequency_list_fcal_iter {m_config.frequencies ()->begin ()},
  m_nTx73 {0},
  m_btxok {false},
  m_diskData {false},
  m_loopall {false},
  m_txFirst {false},
  m_auto {false},
  m_restart {false},
  m_startAnother {false},
  m_saveDecoded {false},
  m_saveAll {false},
  m_widebandDecode {false},
  m_dataAvailable {false},
  m_blankLine {false},
  m_decodedText2 {false},
  m_freeText {false},
  m_sentFirst73 {false},
  m_currentMessageType {-1},
  m_lastMessageType {-1},
  m_bShMsgs {false},
  m_bSWL {false},
  m_uploading {false},
  m_txNext {false},
  m_grid6 {false},
  m_tuneup {false},
  m_bTxTime {false},
  m_rxDone {false},
  m_bSimplex {false},
  m_bEchoTxOK {false},
  m_bTransmittedEcho {false},
  m_bEchoTxed {false},
  m_bFastDecodeCalled {false},
  m_bDoubleClickAfterCQnnn {false},
  m_bRefSpec {false},
  m_bClearRefSpec {false},
  m_bTrain {false},
  m_bAutoReply {false},
  m_QSOProgress {CALLING},
  m_ihsym {0},
  m_nzap {0},
  m_px {0.0},
  m_iptt0 {0},
  m_btxok0 {false},
  m_nsendingsh {0},
  m_onAirFreq0 {0.0},
  m_first_error {true},
  tx_status_label {"Receiving"},
  wsprNet {new WSPRNet {&m_network_manager, this}},
  m_appDir {QApplication::applicationDirPath ()},
  m_palette {"Linrad"},
  m_mode {"FT8"},
  m_rpt {"-15"},
  m_pfx {
    "1A", "1S",
      "3A", "3B6", "3B8", "3B9", "3C", "3C0", "3D2", "3D2C",
      "3D2R", "3DA", "3V", "3W", "3X", "3Y", "3YB", "3YP",
      "4J", "4L", "4S", "4U1I", "4U1U", "4W", "4X",
      "5A", "5B", "5H", "5N", "5R", "5T", "5U", "5V", "5W", "5X", "5Z",
      "6W", "6Y",
      "7O", "7P", "7Q", "7X",
      "8P", "8Q", "8R",
      "9A", "9G", "9H", "9J", "9K", "9L", "9M2", "9M6", "9N",
      "9Q", "9U", "9V", "9X", "9Y",
      "A2", "A3", "A4", "A5", "A6", "A7", "A9", "AP",
      "BS7", "BV", "BV9", "BY",
      "C2", "C3", "C5", "C6", "C9", "CE", "CE0X", "CE0Y",
      "CE0Z", "CE9", "CM", "CN", "CP", "CT", "CT3", "CU",
      "CX", "CY0", "CY9",
      "D2", "D4", "D6", "DL", "DU",
      "E3", "E4", "E5", "EA", "EA6", "EA8", "EA9", "EI", "EK",
      "EL", "EP", "ER", "ES", "ET", "EU", "EX", "EY", "EZ",
      "F", "FG", "FH", "FJ", "FK", "FKC", "FM", "FO", "FOA",
      "FOC", "FOM", "FP", "FR", "FRG", "FRJ", "FRT", "FT5W",
      "FT5X", "FT5Z", "FW", "FY",
      "M", "MD", "MI", "MJ", "MM", "MU", "MW",
      "H4", "H40", "HA", "HB", "HB0", "HC", "HC8", "HH",
      "HI", "HK", "HK0", "HK0M", "HL", "HM", "HP", "HR",
      "HS", "HV", "HZ",
      "I", "IS", "IS0",
      "J2", "J3", "J5", "J6", "J7", "J8", "JA", "JDM",
      "JDO", "JT", "JW", "JX", "JY",
      "K", "KC4", "KG4", "KH0", "KH1", "KH2", "KH3", "KH4", "KH5",
      "KH5K", "KH6", "KH7", "KH8", "KH9", "KL", "KP1", "KP2",
      "KP4", "KP5",
      "LA", "LU", "LX", "LY", "LZ",
      "OA", "OD", "OE", "OH", "OH0", "OJ0", "OK", "OM", "ON",
      "OX", "OY", "OZ",
      "P2", "P4", "PA", "PJ2", "PJ7", "PY", "PY0F", "PT0S", "PY0T", "PZ",
      "R1F", "R1M",
      "S0", "S2", "S5", "S7", "S9", "SM", "SP", "ST", "SU",
      "SV", "SVA", "SV5", "SV9",
      "T2", "T30", "T31", "T32", "T33", "T5", "T7", "T8", "T9", "TA",
      "TF", "TG", "TI", "TI9", "TJ", "TK", "TL", "TN", "TR", "TT",
      "TU", "TY", "TZ",
      "UA", "UA2", "UA9", "UK", "UN", "UR",
      "V2", "V3", "V4", "V5", "V6", "V7", "V8", "VE", "VK", "VK0H",
      "VK0M", "VK9C", "VK9L", "VK9M", "VK9N", "VK9W", "VK9X", "VP2E",
      "VP2M", "VP2V", "VP5", "VP6", "VP6D", "VP8", "VP8G", "VP8H",
      "VP8O", "VP8S", "VP9", "VQ9", "VR", "VU", "VU4", "VU7",
      "XE", "XF4", "XT", "XU", "XW", "XX9", "XZ",
      "YA", "YB", "YI", "YJ", "YK", "YL", "YN", "YO", "YS", "YU", "YV", "YV0",
      "Z2", "Z3", "ZA", "ZB", "ZC4", "ZD7", "ZD8", "ZD9", "ZF", "ZK1N",
      "ZK1S", "ZK2", "ZK3", "ZL", "ZL7", "ZL8", "ZL9", "ZP", "ZS", "ZS8"
      },
  m_sfx {"P",  "0",  "1",  "2",  "3",  "4",  "5",  "6",  "7",  "8",  "9",  "A"},
  mem_jt9 {shdmem},
  m_msAudioOutputBuffered (0u),
  m_framesAudioInputBuffered (RX_SAMPLE_RATE / 10),
  m_downSampleFactor (downSampleFactor),
  m_audioThreadPriority (QThread::HighPriority),
  m_bandEdited {false},
  m_splitMode {false},
  m_monitoring {false},
  m_tx_when_ready {false},
  m_transmitting {false},
  m_tune {false},
  m_tx_watchdog {false},
  m_block_pwr_tooltip {false},
  m_PwrBandSetOK {true},
  m_lastMonitoredFrequency {default_frequency},
  m_toneSpacing {0.},
  m_firstDecode {0},
  m_optimizingProgress {"Optimizing decoder FFTs for your CPU.\n"
      "Please be patient,\n"
      "this may take a few minutes", QString {}, 0, 1, this},
  m_messageClient {new MessageClient {QApplication::applicationName (),
        version (), revision (),
        m_config.udp_server_name (), m_config.udp_server_port (),
        this}},
  m_aprsClient {new APRSISClient{"rotate.aprs2.net", 14580, this}},
  psk_Reporter {new PSK_Reporter {m_messageClient, this}},
  m_i3bit {0},
  m_manual {&m_network_manager},
  m_txFrameCount {0},
  m_previousFreq {0}
{
  ui->setupUi(this);

  createStatusBar();
  add_child_to_event_filter (this);
  ui->dxGridEntry->setValidator (new MaidenheadLocatorValidator {this});
  ui->dxCallEntry->setValidator (new CallsignValidator {this});
  ui->sbTR->values ({5, 10, 15, 30});

  m_baseCall = Radio::base_callsign (m_config.my_callsign ());
  m_opCall = m_config.opCall();

  m_optimizingProgress.setWindowModality (Qt::WindowModal);
  m_optimizingProgress.setAutoReset (false);
  m_optimizingProgress.setMinimumDuration (15000); // only show after 15s delay

  // Closedown.
  connect (ui->actionExit, &QAction::triggered, this, &QMainWindow::close);

  // parts of the rig error message box that are fixed
  m_rigErrorMessageBox.setInformativeText (tr ("Do you want to reconfigure the radio interface?"));
  m_rigErrorMessageBox.setDefaultButton (MessageBox::Ok);

  // start audio thread and hook up slots & signals for shutdown management
  // these objects need to be in the audio thread so that invoking
  // their slots is done in a thread safe way
  m_soundOutput->moveToThread (&m_audioThread);
  m_modulator->moveToThread (&m_audioThread);
  m_soundInput->moveToThread (&m_audioThread);
  m_detector->moveToThread (&m_audioThread);

  // hook up sound output stream slots & signals and disposal
  connect (this, &MainWindow::initializeAudioOutputStream, m_soundOutput, &SoundOutput::setFormat);
  connect (m_soundOutput, &SoundOutput::error, this, &MainWindow::showSoundOutError);
  // connect (m_soundOutput, &SoundOutput::status, this, &MainWindow::showStatusMessage);
  connect (this, &MainWindow::outAttenuationChanged, m_soundOutput, &SoundOutput::setAttenuation);
  connect (&m_audioThread, &QThread::finished, m_soundOutput, &QObject::deleteLater);

  // hook up Modulator slots and disposal
  connect (this, &MainWindow::transmitFrequency, m_modulator, &Modulator::setFrequency);
  connect (this, &MainWindow::endTransmitMessage, m_modulator, &Modulator::stop);
  connect (this, &MainWindow::tune, m_modulator, &Modulator::tune);
  connect (this, &MainWindow::sendMessage, m_modulator, &Modulator::start);
  connect (&m_audioThread, &QThread::finished, m_modulator, &QObject::deleteLater);

  // hook up the audio input stream signals, slots and disposal
  connect (this, &MainWindow::startAudioInputStream, m_soundInput, &SoundInput::start);
  connect (this, &MainWindow::suspendAudioInputStream, m_soundInput, &SoundInput::suspend);
  connect (this, &MainWindow::resumeAudioInputStream, m_soundInput, &SoundInput::resume);
  connect (this, &MainWindow::finished, m_soundInput, &SoundInput::stop);
  connect(m_soundInput, &SoundInput::error, this, &MainWindow::showSoundInError);
  // connect(m_soundInput, &SoundInput::status, this, &MainWindow::showStatusMessage);
  connect (&m_audioThread, &QThread::finished, m_soundInput, &QObject::deleteLater);

  connect (this, &MainWindow::finished, this, &MainWindow::close);

  // hook up the detector signals, slots and disposal
  connect (this, &MainWindow::FFTSize, m_detector, &Detector::setBlockSize);
  connect(m_detector, &Detector::framesWritten, this, &MainWindow::dataSink);
  connect (&m_audioThread, &QThread::finished, m_detector, &QObject::deleteLater);

  // setup the waterfall
  connect(m_wideGraph.data (), SIGNAL(freezeDecode2(int)),this,SLOT(freezeDecode(int)));
  connect(m_wideGraph.data (), SIGNAL(f11f12(int)),this,SLOT(bumpFqso(int)));
  connect(m_wideGraph.data (), SIGNAL(setXIT2(int)),this,SLOT(setXIT(int)));

  connect (m_fastGraph.data (), &FastGraph::fastPick, this, &MainWindow::fastPick);

  connect (this, &MainWindow::finished, m_wideGraph.data (), &WideGraph::close);
  connect (this, &MainWindow::finished, m_echoGraph.data (), &EchoGraph::close);
  connect (this, &MainWindow::finished, m_fastGraph.data (), &FastGraph::close);

  // setup the log QSO dialog
  connect (m_logDlg.data (), &LogQSO::acceptQSO, this, &MainWindow::acceptQSO);
  connect (this, &MainWindow::finished, m_logDlg.data (), &LogQSO::close);


  // Network message handlers
  connect (m_messageClient, &MessageClient::error, this, &MainWindow::networkError);
  connect (m_messageClient, &MessageClient::message, this, &MainWindow::networkMessage);

#if 0
  // Hook up WSPR band hopping
  connect (ui->band_hopping_schedule_push_button, &QPushButton::clicked
           , &m_WSPR_band_hopping, &WSPRBandHopping::show_dialog);
  connect (ui->sbTxPercent, static_cast<void (QSpinBox::*) (int)> (&QSpinBox::valueChanged)
           , &m_WSPR_band_hopping, &WSPRBandHopping::set_tx_percent);
#endif

  on_EraseButton_clicked ();

  QActionGroup* modeGroup = new QActionGroup(this);
  ui->actionFT8->setActionGroup(modeGroup);
  ui->actionJT9->setActionGroup(modeGroup);
  ui->actionJT65->setActionGroup(modeGroup);
  ui->actionJT9_JT65->setActionGroup(modeGroup);
  ui->actionJT4->setActionGroup(modeGroup);
  ui->actionWSPR->setActionGroup(modeGroup);
  ui->actionWSPR_LF->setActionGroup(modeGroup);
  ui->actionEcho->setActionGroup(modeGroup);
  ui->actionISCAT->setActionGroup(modeGroup);
  ui->actionMSK144->setActionGroup(modeGroup);
  ui->actionQRA64->setActionGroup(modeGroup);
  ui->actionFreqCal->setActionGroup(modeGroup);

  QActionGroup* saveGroup = new QActionGroup(this);
  ui->actionNone->setActionGroup(saveGroup);
  ui->actionSave_decoded->setActionGroup(saveGroup);
  ui->actionSave_all->setActionGroup(saveGroup);

  QActionGroup* DepthGroup = new QActionGroup(this);
  ui->actionQuickDecode->setActionGroup(DepthGroup);
  ui->actionMediumDecode->setActionGroup(DepthGroup);
  ui->actionDeepestDecode->setActionGroup(DepthGroup);

  connect (ui->download_samples_action, &QAction::triggered, [this] () {
      if (!m_sampleDownloader)
        {
          m_sampleDownloader.reset (new SampleDownloader {m_settings, &m_config, &m_network_manager, this});
        }
      m_sampleDownloader->show ();
    });

  connect (ui->view_phase_response_action, &QAction::triggered, [this] () {
      if (!m_equalizationToolsDialog)
        {
          m_equalizationToolsDialog.reset (new EqualizationToolsDialog {m_settings, m_config.writeable_data_dir (), m_phaseEqCoefficients, this});
          connect (m_equalizationToolsDialog.data (), &EqualizationToolsDialog::phase_equalization_changed,
                   [this] (QVector<double> const& coeffs) {
                     m_phaseEqCoefficients = coeffs;
                   });
        }
      m_equalizationToolsDialog->show ();
    });

  QButtonGroup* txMsgButtonGroup = new QButtonGroup {this};
  txMsgButtonGroup->addButton(ui->txrb1,1);
  txMsgButtonGroup->addButton(ui->txrb2,2);
  txMsgButtonGroup->addButton(ui->txrb3,3);
  txMsgButtonGroup->addButton(ui->txrb4,4);
  txMsgButtonGroup->addButton(ui->txrb5,5);
  txMsgButtonGroup->addButton(ui->txrb6,6);
  set_dateTimeQSO(-1);
  connect(txMsgButtonGroup,SIGNAL(buttonClicked(int)),SLOT(set_ntx(int)));
  connect (ui->decodedTextBrowser, &DisplayText::selectCallsign, this, &MainWindow::doubleClickOnCall2);
  connect (ui->decodedTextBrowser2, &DisplayText::selectCallsign, this, &MainWindow::doubleClickOnCall);
  connect (ui->textBrowser4, &DisplayText::selectCallsign, this, &MainWindow::doubleClickOnFoxQueue);
  connect (ui->decodedTextBrowser, &DisplayText::erased, this, &MainWindow::band_activity_cleared);
  connect (ui->decodedTextBrowser2, &DisplayText::erased, this, &MainWindow::rx_frequency_activity_cleared);

  // initialize decoded text font and hook up font change signals
  // defer initialization until after construction otherwise menu
  // fonts do not get set
  QTimer::singleShot (0, this, SLOT (initialize_fonts ()));
  connect (&m_config, &Configuration::text_font_changed, [this] (QFont const& font) {
      set_application_font (font);
    });
  connect (&m_config, &Configuration::decoded_text_font_changed, [this] (QFont const& font) {
      setDecodedTextFont (font);
    });
  connect (&m_config, &Configuration::colors_changed, [this](){
     ui->textEditRX->setStyleSheet(QString("QTextEdit{ background: %1 }").arg(m_config.color_ReceivedMsg().name()));
  });

  setWindowTitle (program_title ());

  connect(&proc_jt9, &QProcess::readyReadStandardOutput, this, &MainWindow::readFromStdout);
  connect(&proc_jt9, static_cast<void (QProcess::*) (QProcess::ProcessError)> (&QProcess::error),
          [this] (QProcess::ProcessError error) {
            subProcessError (&proc_jt9, error);
          });
  connect(&proc_jt9, static_cast<void (QProcess::*) (int, QProcess::ExitStatus)> (&QProcess::finished),
          [this] (int exitCode, QProcess::ExitStatus status) {
            subProcessFailed (&proc_jt9, exitCode, status);
          });

  connect(&p1, &QProcess::readyReadStandardOutput, this, &MainWindow::p1ReadFromStdout);
  connect(&p1, static_cast<void (QProcess::*) (QProcess::ProcessError)> (&QProcess::error),
          [this] (QProcess::ProcessError error) {
            subProcessError (&p1, error);
          });
  connect(&p1, static_cast<void (QProcess::*) (int, QProcess::ExitStatus)> (&QProcess::finished),
          [this] (int exitCode, QProcess::ExitStatus status) {
            subProcessFailed (&p1, exitCode, status);
          });

  connect(&p3, static_cast<void (QProcess::*) (QProcess::ProcessError)> (&QProcess::error),
          [this] (QProcess::ProcessError error) {
            subProcessError (&p3, error);
          });
  connect(&p3, static_cast<void (QProcess::*) (int, QProcess::ExitStatus)> (&QProcess::finished),
          [this] (int exitCode, QProcess::ExitStatus status) {
            subProcessFailed (&p3, exitCode, status);
          });

  // hook up save WAV file exit handling
  connect (&m_saveWAVWatcher, &QFutureWatcher<QString>::finished, [this] {
      // extract the promise from the future
      auto const& result = m_saveWAVWatcher.future ().result ();
      if (!result.isEmpty ())   // error
        {
          MessageBox::critical_message (this, tr("Error Writing WAV File"), result);
        }
    });

  // Hook up working frequencies.
  ui->bandComboBox->setModel (m_config.frequencies ());
  ui->bandComboBox->setModelColumn (FrequencyList_v2::frequency_mhz_column);

  // combo box drop down width defaults to the line edit + decorator width,
  // here we change that to the column width size hint of the model column
  ui->bandComboBox->view ()->setMinimumWidth (ui->bandComboBox->view ()->sizeHintForColumn (FrequencyList_v2::frequency_mhz_column));

  // Enable live band combo box entry validation and action.
  auto band_validator = new LiveFrequencyValidator {ui->bandComboBox
                                                    , m_config.bands ()
                                                    , m_config.frequencies ()
                                                    , &m_freqNominal
                                                    , this};
  ui->bandComboBox->setValidator (band_validator);

  // Hook up signals.
  connect (band_validator, &LiveFrequencyValidator::valid, this, &MainWindow::band_changed);
  connect (ui->bandComboBox->lineEdit (), &QLineEdit::textEdited, [this] (QString const&) {m_bandEdited = true;});

  // hook up configuration signals
  connect (&m_config, &Configuration::transceiver_update, this, &MainWindow::handle_transceiver_update);
  connect (&m_config, &Configuration::transceiver_failure, this, &MainWindow::handle_transceiver_failure);
  connect (&m_config, &Configuration::udp_server_changed, m_messageClient, &MessageClient::set_server);
  connect (&m_config, &Configuration::udp_server_port_changed, m_messageClient, &MessageClient::set_server_port);
  connect (&m_config, &Configuration::band_schedule_changed, this, [this](){
    this->m_bandHopped = true;
  });

  // set up configurations menu
  connect (m_multi_settings, &MultiSettings::configurationNameChanged, [this] (QString const& name) {
      if ("Default" != name) {
        config_label.setText (name);
        config_label.show ();
      }
      else {
        config_label.hide ();
      }
    });
  m_multi_settings->create_menu_actions (this, ui->menuConfig);
  m_configurations_button = m_rigErrorMessageBox.addButton (tr ("Configurations...")
                                                            , QMessageBox::ActionRole);

  // set up message text validators
  ui->tx1->setValidator (new QRegExpValidator {message_alphabet, this});
  ui->tx2->setValidator (new QRegExpValidator {message_alphabet, this});
  ui->tx3->setValidator (new QRegExpValidator {message_alphabet, this});
  ui->tx4->setValidator (new QRegExpValidator {message_alphabet, this});
  ui->tx5->setValidator (new QRegExpValidator {message_alphabet, this});
  ui->tx6->setValidator (new QRegExpValidator {message_alphabet, this});
  ui->freeTextMsg->setValidator (new QRegExpValidator {message_alphabet, this});
  ui->nextFreeTextMsg->setValidator (new QRegExpValidator {message_alphabet, this});
  //ui->extFreeTextMsg->setValidator (new QRegExpValidator {message_alphabet, this});

  // Free text macros model to widget hook up.
  //ui->tx5->setModel (m_config.macros ());
  //connect (ui->tx5->lineEdit(), &QLineEdit::editingFinished,
  //         [this] () {on_tx5_currentTextChanged (ui->tx5->lineEdit()->text());});
  //ui->freeTextMsg->setModel (m_config.macros ());
  connect (ui->freeTextMsg->lineEdit ()
           , &QLineEdit::editingFinished
           , [this] () {on_freeTextMsg_currentTextChanged (ui->freeTextMsg->lineEdit ()->text ());});
  connect (ui->nextFreeTextMsg
           , &QLineEdit::editingFinished
           , [this] () {on_nextFreeTextMsg_currentTextChanged (ui->nextFreeTextMsg->text ());});
  connect (ui->extFreeTextMsgEdit
           , &QTextEdit::textChanged
           , [this] () {on_extFreeTextMsgEdit_currentTextChanged (ui->extFreeTextMsgEdit->toPlainText ());});

  connect(&m_guiTimer, &QTimer::timeout, this, &MainWindow::guiUpdate);
  m_guiTimer.start(100);   //### Don't change the 100 ms! ###

  ptt0Timer.setSingleShot(true);
  connect(&ptt0Timer, &QTimer::timeout, this, &MainWindow::stopTx2);

  ptt1Timer.setSingleShot(true);
  connect(&ptt1Timer, &QTimer::timeout, this, &MainWindow::startTx2);

  p1Timer.setSingleShot(true);
  connect(&p1Timer, &QTimer::timeout, this, &MainWindow::startP1);

  logQSOTimer.setSingleShot(true);
  connect(&logQSOTimer, &QTimer::timeout, this, &MainWindow::on_logQSOButton_clicked);

  tuneButtonTimer.setSingleShot(true);
  connect(&tuneButtonTimer, &QTimer::timeout, this, &MainWindow::on_stopTxButton_clicked);

  tuneATU_Timer.setSingleShot(true);
  connect(&tuneATU_Timer, &QTimer::timeout, this, &MainWindow::stopTuneATU);

  killFileTimer.setSingleShot(true);
  connect(&killFileTimer, &QTimer::timeout, this, &MainWindow::killFile);

  uploadTimer.setSingleShot(true);
  connect(&uploadTimer, SIGNAL(timeout()), this, SLOT(uploadSpots()));

  TxAgainTimer.setSingleShot(true);
  connect(&TxAgainTimer, SIGNAL(timeout()), this, SLOT(TxAgain()));

  beaconTimer.setSingleShot(false);
  connect(&beaconTimer, &QTimer::timeout, this, &MainWindow::checkBacon);

  connect(m_wideGraph.data (), SIGNAL(setFreq3(int,int)),this,
          SLOT(setFreq4(int,int)));

  decodeBusy(false);
  QString t1[28]={"1 uW","2 uW","5 uW","10 uW","20 uW","50 uW","100 uW","200 uW","500 uW",
                  "1 mW","2 mW","5 mW","10 mW","20 mW","50 mW","100 mW","200 mW","500 mW",
                  "1 W","2 W","5 W","10 W","20 W","50 W","100 W","200 W","500 W","1 kW"};

  m_msg[0][0]=0;
  m_bQRAsyncWarned=false;
  ui->labDXped->setVisible(false);

  for(int i=0; i<28; i++)  {                      //Initialize dBm values
    float dbm=(10.0*i)/3.0 - 30.0;
    int ndbm=0;
    if(dbm<0) ndbm=int(dbm-0.5);
    if(dbm>=0) ndbm=int(dbm+0.5);
    QString t;
    t.sprintf("%d dBm  ",ndbm);
    t+=t1[i];
    ui->TxPowerComboBox->addItem(t);
  }

  ui->labAz->setStyleSheet("border: 0px;");
//  ui->labDist->setStyleSheet("border: 0px;");

  auto t = "UTC   dB   DT Freq    Message";
  ui->decodedTextLabel->setText(t);
  ui->decodedTextLabel2->setText(t);
  readSettings();            //Restore user's setup params
  m_audioThread.start (m_audioThreadPriority);

#ifdef WIN32
  if (!m_multiple)
    {
      while(true)
        {
          int iret=killbyname("jt9.exe");
          if(iret == 603) break;
          if(iret != 0)
            MessageBox::warning_message (this, tr ("Error Killing jt9.exe Process")
                                         , tr ("KillByName return code: %1")
                                         .arg (iret));
        }
    }
#endif

  {
    //delete any .quit file that might have been left lying around
    //since its presence will cause jt9 to exit a soon as we start it
    //and decodes will hang
    QFile quitFile {m_config.temp_dir ().absoluteFilePath (".quit")};
    while (quitFile.exists ())
      {
        if (!quitFile.remove ())
          {
            MessageBox::query_message (this, tr ("Error removing \"%1\"").arg (quitFile.fileName ())
                                       , tr ("Click OK to retry"));
          }
      }
  }

  //Create .lock so jt9 will wait
  QFile {m_config.temp_dir ().absoluteFilePath (".lock")}.open(QIODevice::ReadWrite);

  QStringList jt9_args {
    "-s", QApplication::applicationName () // shared memory key,
                                           // includes rig
#ifdef NDEBUG
      , "-w", "1"               //FFTW patience - release
#else
      , "-w", "1"               //FFTW patience - debug builds for speed
#endif
      // The number  of threads for  FFTW specified here is  chosen as
      // three because  that gives  the best  throughput of  the large
      // FFTs used  in jt9.  The count  is the minimum of  (the number
      // available CPU threads less one) and three.  This ensures that
      // there is always at least one free CPU thread to run the other
      // mode decoder in parallel.
      , "-m", QString::number (qMin (qMax (QThread::idealThreadCount () - 1, 1), 3)) //FFTW threads

      , "-e", QDir::toNativeSeparators (m_appDir)
      , "-a", QDir::toNativeSeparators (m_config.writeable_data_dir ().absolutePath ())
      , "-t", QDir::toNativeSeparators (m_config.temp_dir ().absolutePath ())
      };
  QProcessEnvironment env {QProcessEnvironment::systemEnvironment ()};
  env.insert ("OMP_STACKSIZE", "4M");
  proc_jt9.setProcessEnvironment (env);
  proc_jt9.start(QDir::toNativeSeparators (m_appDir) + QDir::separator () +
          "jt9", jt9_args, QIODevice::ReadWrite | QIODevice::Unbuffered);

  QString fname {QDir::toNativeSeparators(m_config.writeable_data_dir ().absoluteFilePath ("wsjtx_wisdom.dat"))};
  QByteArray cfname=fname.toLocal8Bit();
  fftwf_import_wisdom_from_filename(cfname);

  //genStdMsgs(m_rpt);
  m_ntx = 6;
  ui->txrb6->setChecked(true);

  connect (&m_wav_future_watcher, &QFutureWatcher<void>::finished, this, &MainWindow::diskDat);

  connect(&watcher3, SIGNAL(finished()),this,SLOT(fast_decode_done()));
//  Q_EMIT startAudioInputStream (m_config.audio_input_device (), m_framesAudioInputBuffered, &m_detector, m_downSampleFactor, m_config.audio_input_channel ());
  Q_EMIT startAudioInputStream (m_config.audio_input_device (), m_framesAudioInputBuffered, m_detector, m_downSampleFactor, m_config.audio_input_channel ());
  Q_EMIT initializeAudioOutputStream (m_config.audio_output_device (), AudioDevice::Mono == m_config.audio_output_channel () ? 1 : 2, m_msAudioOutputBuffered);
  Q_EMIT transmitFrequency (ui->TxFreqSpinBox->value () - m_XIT);

  enable_DXCC_entity (m_config.DXCC ());  // sets text window proportions and (re)inits the logbook

  ui->label_9->setStyleSheet("QLabel{background-color: #aabec8}");
  ui->label_10->setStyleSheet("QLabel{background-color: #aabec8}");

  // this must be done before initializing the mode as some modes need
  // to turn off split on the rig e.g. WSPR
  m_config.transceiver_online ();
  bool vhf {m_config.enable_VHF_features ()};

  ui->txFirstCheckBox->setChecked(m_txFirst);
  morse_(const_cast<char *> (m_config.my_callsign ().toLatin1().constData()),
         const_cast<int *> (icw), &m_ncw, m_config.my_callsign ().length());
  on_actionWide_Waterfall_triggered();
  ui->cbShMsgs->setChecked(m_bShMsgs);
  ui->cbSWL->setChecked(m_bSWL);
  if(m_bFast9) m_bFastMode=true;
  ui->cbFast9->setChecked(m_bFast9 or m_bFastMode);

  if(true || m_mode=="FT8") on_actionFT8_triggered();

  ui->sbSubmode->setValue (vhf ? m_nSubMode : 0);
  if(m_mode=="MSK144") {
    Q_EMIT transmitFrequency (1000.0);
  } else {
    Q_EMIT transmitFrequency (ui->TxFreqSpinBox->value() - m_XIT);
  }
  m_saveDecoded=ui->actionSave_decoded->isChecked();
  m_saveAll=ui->actionSave_all->isChecked();
  ui->sbTxPercent->setValue(m_pctx);
  ui->TxPowerComboBox->setCurrentIndex(int(0.3*(m_dBm + 30.0)+0.2));
  ui->cbUploadWSPR_Spots->setChecked(m_uploadSpots);
  if((m_ndepth&7)==1) ui->actionQuickDecode->setChecked(true);
  if((m_ndepth&7)==2) ui->actionMediumDecode->setChecked(true);
  if((m_ndepth&7)==3) ui->actionDeepestDecode->setChecked(true);
  ui->actionInclude_averaging->setChecked(m_ndepth&16);
  ui->actionInclude_correlation->setChecked(m_ndepth&32);
  ui->actionEnable_AP_DXcall->setChecked(m_ndepth&64);

  m_UTCdisk=-1;
  m_fCPUmskrtd=0.0;
  m_bFastDone=false;
  m_bAltV=false;
  m_bNoMoreFiles=false;
  m_bVHFwarned=false;
  m_bDoubleClicked=false;
  m_bCallingCQ=false;
  m_bCheckedContest=false;
  m_bDisplayedOnce=false;
  m_wait=0;
  m_isort=-3;
  m_max_dB=30;
  m_CQtype="CQ";

  if(m_mode.startsWith ("WSPR") and m_pctx>0)  {
    QPalette palette {ui->sbTxPercent->palette ()};
    palette.setColor(QPalette::Base,Qt::yellow);
    ui->sbTxPercent->setPalette(palette);
  }
  fixStop();
  VHF_features_enabled(m_config.enable_VHF_features());
  m_wideGraph->setVHF(m_config.enable_VHF_features());

  connect( wsprNet, SIGNAL(uploadStatus(QString)), this, SLOT(uploadResponse(QString)));

  statusChanged();

  m_fastGraph->setMode(m_mode);
  m_wideGraph->setMode(m_mode);
  m_wideGraph->setModeTx(m_modeTx);

  connect (&minuteTimer, &QTimer::timeout, this, &MainWindow::on_the_minute);
  minuteTimer.setSingleShot (true);
  minuteTimer.start (ms_minute_error () + 60 * 1000);

  //connect (&splashTimer, &QTimer::timeout, this, &MainWindow::splash_done);
  //splashTimer.setSingleShot (true);
  //splashTimer.start (20 * 1000);

/*
  if(m_config.my_callsign()=="K1JT" or m_config.my_callsign()=="K9AN" or
     m_config.my_callsign()=="G4WJS" or
     m_config.my_callsign().contains("KH7Z")) {
    ui->actionWSPR_LF->setEnabled(true);
  } else {
    QString errorMsg;
    MessageBox::critical_message (this,
       "Code in the WSJT-X development branch is\n"
       "not currently available for on-the-air use.\n\n"
       "Please use WSJT-X v1.8.0\n", errorMsg);
    Q_EMIT finished ();
  }
*/

  /*
  if(QCoreApplication::applicationVersion().contains("-devel") or
     QCoreApplication::applicationVersion().contains("-rc")) {
    QTimer::singleShot (0, this, SLOT (not_GA_warning_message ()));
  }
  */

  // TODO: jsherer - need to remove this eventually...
  QTimer::singleShot (0, this, SLOT (not_GA_warning_message ()));

  if(!ui->cbMenus->isChecked()) {
    ui->cbMenus->setChecked(true);
    ui->cbMenus->setChecked(false);
  }

  //UI Customizations
  m_wideGraph.data()->installEventFilter(new EscapeKeyPressEater());
  ui->mdiArea->addSubWindow(m_wideGraph.data(), Qt::Dialog | Qt::FramelessWindowHint | Qt::CustomizeWindowHint | Qt::Tool)->showMaximized();
  ui->menuDecode->setEnabled(false);
  ui->menuMode->setVisible(false);
  ui->menuSave->setEnabled(true);
  ui->menuTools->setEnabled(false);
  ui->menuView->setEnabled(false);
  foreach(auto action, ui->menuBar->actions()){
      if(action->text() == "View") ui->menuBar->removeAction(action);
      if(action->text() == "Mode") ui->menuBar->removeAction(action);
      if(action->text() == "Decode") ui->menuBar->removeAction(action);
      if(action->text() == "Tools") ui->menuBar->removeAction(action);
  }
  ui->dxCallEntry->clear();
  ui->dxGridEntry->clear();
  auto f = findFreeFreqOffset(500, 2000, 50);
  setFreqOffsetForRestore(f, false);
  ui->cbVHFcontest->setChecked(false); // this needs to always be false

  ui->spotButton->setChecked(m_config.spot_to_reporting_networks());

  auto enterFilter = new EnterKeyPressEater();
  connect(enterFilter, &EnterKeyPressEater::enterKeyPressed, this, [this](QObject *, QKeyEvent *, bool *pProcessed){
      if(QApplication::keyboardModifiers() & Qt::ShiftModifier){
          if(pProcessed) *pProcessed = false;
          return;
      }
      if(ui->extFreeTextMsgEdit->isReadOnly()){
          if(pProcessed) *pProcessed = false;
          return;
      }

      if(pProcessed) *pProcessed = true;

      if(ui->extFreeTextMsgEdit->toPlainText().trimmed().isEmpty()){
          return;
      }

      toggleTx(true);
  });
  ui->extFreeTextMsgEdit->installEventFilter(enterFilter);

  auto clearActionSep = new QAction(nullptr);
  clearActionSep->setSeparator(true);

  auto clearActionAll = new QAction(QIcon::fromTheme("edit-clear"), QString("Clear All"), nullptr);
  connect(clearActionAll, &QAction::triggered, this, &MainWindow::clearActivity);

  // setup tablewidget context menus
  auto clearAction1 = new QAction(QIcon::fromTheme("edit-clear"), QString("Clear"), ui->textEditRX);
  connect(clearAction1, &QAction::triggered, this, [this](){ this->on_clearAction_triggered(ui->textEditRX); });
  ui->textEditRX->setContextMenuPolicy(Qt::ActionsContextMenu);
  ui->textEditRX->addAction(clearAction1);
  ui->textEditRX->addAction(clearActionAll);



  auto clearAction2 = new QAction(QIcon::fromTheme("edit-clear"), QString("Clear"), ui->extFreeTextMsgEdit);
  connect(clearAction2, &QAction::triggered, this, [this](){ this->on_clearAction_triggered(ui->extFreeTextMsgEdit); });
  ui->extFreeTextMsgEdit->setContextMenuPolicy(Qt::ActionsContextMenu);

  auto restoreAction = new QAction(QString("Restore Previous Message"), ui->extFreeTextMsgEdit);
  connect(restoreAction, &QAction::triggered, this, [this](){ this->restoreMessage(); });

  ui->extFreeTextMsgEdit->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(ui->extFreeTextMsgEdit, &QTableWidget::customContextMenuRequested, this, [this, clearAction2, clearActionAll, restoreAction](QPoint const &point){
    QMenu * menu = new QMenu(ui->extFreeTextMsgEdit);

    restoreAction->setDisabled(m_lastTxMessage.isEmpty());
    menu->addAction(restoreAction);

    menu->addSeparator();
    menu->addAction(clearAction2);
    menu->addAction(clearActionAll);

    menu->popup(ui->extFreeTextMsgEdit->mapToGlobal(point));
  });



  auto clearAction3 = new QAction(QIcon::fromTheme("edit-clear"), QString("Clear"), ui->tableWidgetRXAll);
  connect(clearAction3, &QAction::triggered, this, [this](){ this->on_clearAction_triggered(ui->tableWidgetRXAll); });

  auto removeActivity = new QAction(QString("Remove Activity"), ui->tableWidgetRXAll);
  connect(removeActivity, &QAction::triggered, this, [this](){
      if(ui->tableWidgetRXAll->selectedItems().isEmpty()){
          return;
      }

      auto selectedItems = ui->tableWidgetRXAll->selectedItems();
      int selectedOffset = selectedItems.first()->text().toInt();

      m_bandActivity.remove(selectedOffset);
      displayActivity(true);
  });

  auto logAction = new QAction(QString("Log..."), ui->tableWidgetCalls);
  connect(logAction, &QAction::triggered, this, &MainWindow::on_logQSOButton_clicked);



  ui->tableWidgetRXAll->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(ui->tableWidgetRXAll, &QTableWidget::customContextMenuRequested, this, [this, clearAction3, clearActionAll, removeActivity, logAction](QPoint const &point){
    QMenu * menu = new QMenu(ui->tableWidgetRXAll);

    // clear the selection of the call widget on right click
    ui->tableWidgetCalls->selectionModel()->clearSelection();

    QString selectedCall = callsignSelected();
    bool missingCallsign = selectedCall.isEmpty();

    int selectedOffset = -1;
    if(!ui->tableWidgetRXAll->selectedItems().isEmpty()){
        auto selectedItems = ui->tableWidgetRXAll->selectedItems();
        selectedOffset = selectedItems.first()->text().toInt();
    }

    menu->addAction(logAction);
    logAction->setDisabled(missingCallsign);

    auto directedMenu = menu->addMenu(QString("Directed to %1...").arg(selectedCall));
    directedMenu->setDisabled(missingCallsign);
    buildQueryMenu(directedMenu, selectedCall);

    auto deselect = menu->addAction("Deselect");
    deselect->setDisabled(missingCallsign);
    connect(deselect, &QAction::triggered, this, [this](){
        ui->tableWidgetRXAll->clearSelection();
        ui->tableWidgetCalls->clearSelection();
    });

    menu->addSeparator();

    removeActivity->setDisabled(selectedOffset == -1);
    menu->addAction(removeActivity);


    menu->addSeparator();

    auto sortMenu = menu->addMenu(QString("Sort by..."));
    buildBandActivitySortByMenu(sortMenu);

    menu->addSeparator();
    menu->addAction(clearAction3);
    menu->addAction(clearActionAll);

    menu->popup(ui->tableWidgetRXAll->mapToGlobal(point));
  });




  auto clearAction4 = new QAction(QIcon::fromTheme("edit-clear"), QString("Clear"), ui->tableWidgetCalls);
  connect(clearAction4, &QAction::triggered, this, [this](){ this->on_clearAction_triggered(ui->tableWidgetCalls); });

  auto removeStation = new QAction(QString("Remove Station"), ui->tableWidgetCalls);
  connect(removeStation, &QAction::triggered, this, [this](){
      QString selectedCall = callsignSelected();
      if(!selectedCall.isEmpty() && m_callActivity.contains(selectedCall)){
          m_callActivity.remove(selectedCall);
          displayActivity(true);
      }
  });


  ui->tableWidgetCalls->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(ui->tableWidgetCalls, &QTableWidget::customContextMenuRequested, this, [this, logAction, clearAction4, clearActionAll, removeStation](QPoint const &point){
    QMenu * menu = new QMenu(ui->tableWidgetCalls);

    ui->tableWidgetRXAll->selectionModel()->clearSelection();

    QString selectedCall = callsignSelected();
    bool isAllCall = isAllCallIncluded(selectedCall);
    bool missingCallsign = selectedCall.isEmpty();
    if(!missingCallsign && m_callActivity.contains(selectedCall)){
        setFreqOffsetForRestore(m_callActivity[selectedCall].freq, true);
    }

    menu->addAction(logAction);
    logAction->setDisabled(missingCallsign || isAllCall);

    auto directedMenu = menu->addMenu(QString("Directed to %1...").arg(selectedCall));
    directedMenu->setDisabled(missingCallsign);
    buildQueryMenu(directedMenu, selectedCall);

    auto deselect = menu->addAction("Deselect");
    deselect->setDisabled(missingCallsign);
    connect(deselect, &QAction::triggered, this, [this](){
        ui->tableWidgetRXAll->clearSelection();
        ui->tableWidgetCalls->clearSelection();
    });

    menu->addSeparator();

    removeStation->setDisabled(missingCallsign || isAllCall);
    menu->addAction(removeStation);

    menu->addSeparator();

    auto sortMenu = menu->addMenu(QString("Sort by..."));
    buildCallActivitySortByMenu(sortMenu);

    menu->addSeparator();
    menu->addAction(clearAction4);
    menu->addAction(clearActionAll);

    menu->popup(ui->tableWidgetCalls->mapToGlobal(point));
  });

  connect(ui->tableWidgetRXAll->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::on_tableWidgetRXAll_selectionChanged);
  connect(ui->tableWidgetCalls->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::on_tableWidgetCalls_selectionChanged);


  // Don't block beacon's first run...
  m_lastTxTime = QDateTime::currentDateTimeUtc().addSecs(-300);

  displayActivity(true);

#if 0
  // TESTING :P
  qint64 a = QDateTime::currentSecsSinceEpoch();
  qDebug() << a << Varicode::pack64bits(a) << Varicode::unpack64bits(Varicode::pack64bits(a));
  qDebug() << a << Varicode::bitsToStr(Varicode::intToBits(a)) << Varicode::bitsToInt(Varicode::strToBits(Varicode::bitsToStr(Varicode::intToBits(a))));
  auto input = "HELLO BRAVE NEW WORLD!\x04";
  auto encoded = Varicode::huffEncode(input) + QList<QVector<bool>> {Varicode::strToBits("000000000")};
  auto decoded = Varicode::huffDecode(Varicode::huffFlatten(encoded));
  qDebug() << input << Varicode::bitsToStr(Varicode::huffFlatten(encoded)) << decoded;
  qDebug() << Varicode::packCallsign("JY1") << Varicode::unpackCallsign(Varicode::packCallsign("JY1"));
  auto frames = buildFT8MessageFrames("OH8STN:KN4CRD?");
  qDebug() << frames.first() << Varicode::unpackDirectedMessage(frames.first());
  qDebug() << Varicode::packGrid("EM73tu") << Varicode::unpackGrid(Varicode::packGrid("EM73tu"));

  auto allbits = Varicode::huffEncode(QString("When simple things need instructions, it is a certain sign of poor design.\x04").toUpper());
  qDebug() << "FTFrames" << qCeil((allbits.length()+(6+6+1))/13.0);
  int lasti = 0;
  int i = 0;
  int frames = 0;
  QVector<bool> nextFrame;
  foreach(auto bits, allbits){
      if(nextFrame.length() + bits.length() > (frames == 0 ? (8-3) : 64)){
          // emit frame
          qDebug() << "frame ready at" << i << "chars" << i-lasti << "size" << nextFrame.length() << "->" << Varicode::bitsToStr(nextFrame);
          nextFrame.clear();
          lasti = i;
          frames++;
      }

      nextFrame += bits;
      i++;
  }
  qDebug() << "frame ready at" << i << "size" << nextFrame.length() << "->" << Varicode::bitsToStr(nextFrame);
  frames++;
  qDebug() << "HuffFrames" << frames;
  qDebug() << Varicode::packCallsignPrefixSuffix("VE3") << Varicode::unpackCallsignPrefixSuffix(Varicode::packCallsignPrefixSuffix("VE3");

  auto calls = Varicode::parseCallsigns("VE3/KN4CRD 9E/KN4CRD KN4CRD/7 KN4CRD/P KN4CRD");
  foreach(auto call, calls){
    qDebug() << call << Radio::base_callsign(call) << QString(call).replace(Radio::base_callsign(call), "");

    auto base = Radio::base_callsign(call);
    auto fix = call.replace(base, "").replace("/", "");
    qDebug() << fix;

    auto packedCall = ((quint64)Varicode::packCallsign(base) << 32) | Varicode::packCallsignPrefixSuffix(fix);

    auto packed = Varicode::pack64bits(packedCall);

    qDebug() << call << packedCall << packed;
  }

  auto call = QString(m_config.my_callsign());
  qDebug() << call;
  auto basecall = Radio::base_callsign(call);
  qDebug() << call << basecall;
  auto fix = QString(call).replace(basecall, "");
  qDebug() << call << basecall << fix;
  auto prefix = !fix.startsWith("/");
  fix = fix.replace("/", "");

  auto packed = Varicode::packCompoundMessage(basecall, fix, prefix, 99);
  qDebug() << packed << Varicode::unpackCompoundMessage(packed);

  bool isCQ = false;

  auto packed = Varicode::packBeaconMessage("KN4CRD/P", "EM73", true);
  qDebug() << packed << Varicode::unpackBeaconMessage(packed, &isCQ) << isCQ;

  packed = Varicode::packBeaconMessage("VE3/KN4CRD", "EM73", false);
  qDebug() << packed << Varicode::unpackBeaconMessage(packed, &isCQ) << isCQ;


  bool isCQ = false;

  auto packed = Varicode::packBeaconMessage("P/KN4CRD", "", true);
  qDebug() << packed << Varicode::unpackBeaconMessage(packed, &isCQ) << isCQ;

  m_valid = false;

  bool isPrefix = false;
  qDebug() << Varicode::packCallsignPrefixSuffix("VE3", true) << Varicode::unpackCallsignPrefixSuffix(Varicode::packCallsignPrefixSuffix("VE3", true), &isPrefix) << isPrefix;


#define BIT (quint64)1
  quint64 val = 0;
  quint8 rem = 0;
  quint64 j = (BIT<<40)-1;
  quint64 k = (BIT<<5)-1;

  qDebug() << j << k << val << rem;
  qDebug() << "packing" << Varicode::unpack72bits(Varicode::pack72bits(j, k), &val, &rem);
  qDebug() << j << k << val << rem;

  m_valid = false;

  quint8 r = 0;
  quint64 v = Varicode::unpack72bits(Varicode::pack72bits((((quint64)1)<<62)-1, (1<<7)-1), &r);
  qDebug() << "packing" << Varicode::pack72bits((((quint64)1)<<62)-1, (1<<7)-1) << v << r;

  qDebug() << APRSISClient::grid2deg("EM73");
  qDebug() << APRSISClient::grid2deg("EM73TU");
  qDebug() << APRSISClient::grid2deg("EM73TU49NT");

  qDebug() << APRSISClient::grid2aprs("EM73");
  qDebug() << APRSISClient::grid2aprs("EM73TU");
  qDebug() << APRSISClient::grid2aprs("EM73TU49NT");

  qDebug() << APRSISClient::grid2aprs("FI08VE49");
  qDebug() << APRSISClient::grid2aprs("OM25CU");
#endif

  // this must be the last statement of constructor
  if (!m_valid) throw std::runtime_error {"Fatal initialization exception"};
}

QDate eol(2018, 9, 17);

void MainWindow::expiry_warning_message()
{
    if(QDate::currentDate() >= eol){
        MessageBox::critical_message (this, QString("This pre-release development build of FT8Call has expired. Please upgrade to the latest version."));
        close();
        return;
    }
}

void MainWindow::not_GA_warning_message ()
{
  expiry_warning_message();

  MessageBox::critical_message (this,
                                QString("This version of %1 is a pre-release development\n"
                                "build and will expire on %2. \n\n"
                                "Use of development versions of FT8Call are at your own risk \n"
                                "and carry a responsiblity to report any problems to:\n"
                                "Jordan Sherer (KN4CRD) kn4crd@gmail.com\n\n").arg(QApplication::applicationName()).arg(eol.toString()));


  ensureCallsignSet(false);

  CallDetail cd1 = {};
  cd1.call = "KN4CRD/P";
  m_callActivity["KN4CRD/P"] = cd1;

  CallDetail cd2 = {};
  cd2.call = "KN4CRD/QRP";
  m_callActivity["KN4CRD/QRP"] = cd2;

  CallDetail cd3 = {};
  cd3.call = "OH8STN/QRP";
  m_callActivity["OH8STN/QRP"] = cd3;

  displayActivity(true);
}

void MainWindow::initialize_fonts ()
{
  set_application_font (m_config.text_font ());
  setDecodedTextFont (m_config.decoded_text_font ());
}

void MainWindow::splash_done ()
{
  m_splash && m_splash->close ();
}

void MainWindow::on_the_minute ()
{
  if (minuteTimer.isSingleShot ())
    {
      minuteTimer.setSingleShot (false);
      minuteTimer.start (60 * 1000); // run free
    }
  else
    {
        auto const& ms_error = ms_minute_error ();
        if (qAbs (ms_error) > 1000) // keep drift within +-1s
        {
          minuteTimer.setSingleShot (true);
          minuteTimer.start (ms_error + 60 * 1000);
        }
    }

  if (m_config.watchdog () && !m_mode.startsWith ("WSPR"))
    {
      if (m_idleMinutes < m_config.watchdog ()) ++m_idleMinutes;
      update_watchdog_label ();
    }
  else
    {
      tx_watchdog (false);
    }
}

void MainWindow::tryBandHop(){
  // see if we need to hop bands...
  if(!m_config.auto_switch_bands()){
      return;
  }

  // make sure we're not transmitting
  if(isMessageQueuedForTransmit()){
      return;
  }

  // get the current band
  auto dialFreq = dialFrequency();

  auto currentBand = m_config.bands()->find(dialFreq);

  // get the stations list
  auto stations = m_config.stations()->station_list();

  // order stations by (switch_at, switch_until) time tuple
  qSort(stations.begin(), stations.end(), [](StationList::Station const &a, StationList::Station const &b){
    return (a.switch_at_ < b.switch_at_) || (a.switch_at_ == b.switch_at_ && a.switch_until_ < b.switch_until_);
  });

  // we just set the date to a known y/m/d to make the comparisons easier
  QDateTime d = QDateTime::currentDateTimeUtc();
  d.setDate(QDate(2000, 1, 1));

  QDateTime startOfDay = QDateTime(QDate(2000, 1, 1), QTime(0, 0));
  QDateTime endOfDay = QDateTime(QDate(2000, 1, 1), QTime(23, 59));

  // see if we can find a needed band switch...
  foreach(auto station, stations){
      // we can switch to this frequency if we're in the time range, inclusive of switch_at, exclusive of switch_until
      // and if we are switching to a different frequency than the last hop. this allows us to switch bands at that time,
      // but then later we can later switch to a different band if needed without the automatic band switching to take over
      bool inTimeRange = (
        (station.switch_at_ <= d && d <= station.switch_until_) ||          // <- normal range, 12-16 && 6-8, evalued as 12 <= d <= 16 || 6 <= d <= 8

        (station.switch_until_ < station.switch_at_ && (                    // <- say for a range of 12->2 & 2->12;  12->2,
             (station.switch_at_ <= d && d <= endOfDay)         ||          //    should be evaluated as 12 <= d <= 23:59 || 00:00 <= d <= 2
             (startOfDay <= d && d <= station.switch_until_)
        ))
      );

      bool noOverride = (
        (m_bandHopped || (!m_bandHopped && station.frequency_ != m_bandHoppedFreq))
      );

      bool freqIsDifferent = (station.frequency_ != dialFreq);

      bool canSwitch = (
        inTimeRange     &&
        noOverride      &&
        freqIsDifferent
      );

      // switch, if we can and the band is different than our current band
      if(canSwitch){
          Frequency frequency = station.frequency_;

          m_bandHopped = false;
          m_bandHoppedFreq = frequency;

          SelfDestructMessageBox * m = new SelfDestructMessageBox(30,
            "Scheduled Frequency Change",
            QString("A scheduled frequency change has arrived. The rig frequency will be changed to %1 MHz in %2 second(s).").arg(Radio::frequency_MHz_string(station.frequency_)),
            QMessageBox::Information,
            QMessageBox::Ok | QMessageBox::Cancel,
            QMessageBox::Ok,
            this);

          connect(m, &SelfDestructMessageBox::accepted, this, [this, frequency](){
              m_bandHopped = true;
              setRig(frequency);
          });

          m->show();

#if 0
          // TODO: jsherer - this is totally a hack because of the signal that gets emitted to clearActivity on band change...
          QTimer *t = new QTimer(this);
          t->setInterval(250);
          t->setSingleShot(true);
          connect(t, &QTimer::timeout, this, [this, station, dialFreq](){
              auto message = QString("Scheduled frequency switch from %1 MHz to %2 MHz");
              message = message.arg(Radio::frequency_MHz_string(dialFreq));
              message = message.arg(Radio::frequency_MHz_string(station.frequency_));
              writeNoticeTextToUI(QDateTime::currentDateTimeUtc(), message);
          });
          t->start();
#endif

          return;
      }
  }
}

//--------------------------------------------------- MainWindow destructor
MainWindow::~MainWindow()
{
  m_astroWidget.reset ();
  QString fname {QDir::toNativeSeparators(m_config.writeable_data_dir ().absoluteFilePath ("wsjtx_wisdom.dat"))};
  QByteArray cfname=fname.toLocal8Bit();
  fftwf_export_wisdom_to_filename(cfname);
  m_audioThread.quit ();
  m_audioThread.wait ();
  remove_child_from_event_filter (this);
}

//-------------------------------------------------------- writeSettings()
void MainWindow::writeSettings()
{
  m_settings->beginGroup("MainWindow");
  m_settings->setValue ("geometry", saveGeometry ());
  m_settings->setValue ("geometryNoControls", m_geometryNoControls);
  m_settings->setValue ("state", saveState ());
  m_settings->setValue("MRUdir", m_path);
  m_settings->setValue("TxFirst",m_txFirst);
  m_settings->setValue("DXcall",ui->dxCallEntry->text());
  m_settings->setValue("DXgrid",ui->dxGridEntry->text());
  m_settings->setValue ("AstroDisplayed", m_astroWidget && m_astroWidget->isVisible());
  m_settings->setValue ("MsgAvgDisplayed", m_msgAvgWidget && m_msgAvgWidget->isVisible());
  m_settings->setValue ("FreeText", ui->freeTextMsg->currentText ());
  m_settings->setValue("ShowMenus",ui->cbMenus->isChecked());
  m_settings->setValue("CallFirst",ui->cbFirst->isChecked());
  m_settings->setValue("HoundSort",ui->comboBoxHoundSort->currentIndex());
  m_settings->setValue("FoxNlist",ui->sbNlist->value());
  m_settings->setValue("FoxNslots",ui->sbNslots->value());
  m_settings->setValue("FoxMaxDB",ui->sbMax_dB->value());

  m_settings->setValue("MainSplitter", ui->mainSplitter->saveState());
  m_settings->setValue("TextHorizontalSplitter", ui->textHorizontalSplitter->saveState());
  m_settings->setValue("TextVerticalSplitter", ui->textVerticalSplitter->saveState());

  m_settings->endGroup();

  m_settings->beginGroup("Common");
  m_settings->setValue("Mode",m_mode);
  m_settings->setValue("ModeTx",m_modeTx);
  m_settings->setValue("SaveNone",ui->actionNone->isChecked());
  m_settings->setValue("SaveDecoded",ui->actionSave_decoded->isChecked());
  m_settings->setValue("SaveAll",ui->actionSave_all->isChecked());
  m_settings->setValue("NDepth",m_ndepth);
  m_settings->setValue("RxFreq",ui->RxFreqSpinBox->value());
  m_settings->setValue("TxFreq",ui->TxFreqSpinBox->value());
  m_settings->setValue("WSPRfreq",ui->WSPRfreqSpinBox->value());
  m_settings->setValue("SubMode",ui->sbSubmode->value());
  m_settings->setValue("DTtol",m_DTtol);
  m_settings->setValue("Ftol", ui->sbFtol->value ());
  m_settings->setValue("MinSync",m_minSync);
  m_settings->setValue ("AutoSeq", ui->cbAutoSeq->isChecked ());
  m_settings->setValue ("RxAll", ui->cbRxAll->isChecked ());
  m_settings->setValue ("VHFcontest", ui->cbVHFcontest->isChecked ());
  m_settings->setValue("ShMsgs",m_bShMsgs);
  m_settings->setValue("SWL",ui->cbSWL->isChecked());
  m_settings->setValue ("DialFreq", QVariant::fromValue(m_lastMonitoredFrequency));
  m_settings->setValue("OutAttenuation", ui->outAttenuation->value ());
  m_settings->setValue("NoSuffix",m_noSuffix);
  m_settings->setValue("GUItab",ui->tabWidget->currentIndex());
  m_settings->setValue("OutBufSize",outBufSize);
  m_settings->setValue ("HoldTxFreq", ui->cbHoldTxFreq->isChecked ());
  m_settings->setValue("PctTx",m_pctx);
  m_settings->setValue("dBm",m_dBm);
  m_settings->setValue ("WSPRPreferType1", ui->WSPR_prefer_type_1_check_box->isChecked ());
  m_settings->setValue("UploadSpots",m_uploadSpots);
  m_settings->setValue ("BandHopping", ui->band_hopping_group_box->isChecked ());
  m_settings->setValue ("TRPeriod", ui->sbTR->value ());
  m_settings->setValue("FastMode",m_bFastMode);
  m_settings->setValue("Fast9",m_bFast9);
  m_settings->setValue ("CQTxfreq", ui->sbCQTxFreq->value ());
  m_settings->setValue("pwrBandTxMemory",m_pwrBandTxMemory);
  m_settings->setValue("pwrBandTuneMemory",m_pwrBandTuneMemory);
  m_settings->setValue ("FT8AP", ui->actionEnable_AP_FT8->isChecked ());
  m_settings->setValue ("JT65AP", ui->actionEnable_AP_JT65->isChecked ());
  m_settings->setValue("SortBy", QVariant(m_sortCache));



  // TODO: jsherer - need any other customizations?
  /*m_settings->setValue("PanelLeftGeometry", ui->tableWidgetRXAll->geometry());
  m_settings->setValue("PanelRightGeometry", ui->tableWidgetCalls->geometry());
  m_settings->setValue("PanelTopGeometry", ui->extFreeTextMsg->geometry());
  m_settings->setValue("PanelBottomGeometry", ui->extFreeTextMsgEdit->geometry());
  m_settings->setValue("PanelWaterfallGeometry", ui->bandHorizontalWidget->geometry());*/
  //m_settings->setValue("MainSplitter", QVariant::fromValue(ui->mainSplitter->sizes()));

  {
    QList<QVariant> coeffs;     // suitable for QSettings
    for (auto const& coeff : m_phaseEqCoefficients)
      {
        coeffs << coeff;
      }
    m_settings->setValue ("PhaseEqualizationCoefficients", QVariant {coeffs});
  }
  m_settings->endGroup();
}

//---------------------------------------------------------- readSettings()
void MainWindow::readSettings()
{
  ui->cbVHFcontest->setVisible(false);
  ui->cbAutoSeq->setVisible(false);
  ui->cbFirst->setVisible(false);
  m_settings->beginGroup("MainWindow");
  restoreGeometry (m_settings->value ("geometry", saveGeometry ()).toByteArray ());
  setMinimumSize(800, 545);

  m_geometryNoControls = m_settings->value ("geometryNoControls",saveGeometry()).toByteArray();
  restoreState (m_settings->value ("state", saveState ()).toByteArray ());
  ui->dxCallEntry->setText (m_settings->value ("DXcall", QString {}).toString ());
  ui->dxGridEntry->setText (m_settings->value ("DXgrid", QString {}).toString ());
  m_path = m_settings->value("MRUdir", m_config.save_directory ().absolutePath ()).toString ();
  m_txFirst = m_settings->value("TxFirst",false).toBool();
  auto displayAstro = m_settings->value ("AstroDisplayed", false).toBool ();
  auto displayMsgAvg = m_settings->value ("MsgAvgDisplayed", false).toBool ();
  if (m_settings->contains ("FreeText")) ui->freeTextMsg->setCurrentText (
        m_settings->value ("FreeText").toString ());
  ui->cbMenus->setChecked(m_settings->value("ShowMenus",true).toBool());
  ui->cbFirst->setChecked(m_settings->value("CallFirst",true).toBool());
  ui->comboBoxHoundSort->setCurrentIndex(m_settings->value("HoundSort",3).toInt());
  ui->sbNlist->setValue(m_settings->value("FoxNlist",12).toInt());
  ui->sbNslots->setValue(m_settings->value("FoxNslots",5).toInt());
  ui->sbMax_dB->setValue(m_settings->value("FoxMaxDB",30).toInt());

  auto mainSplitterState = m_settings->value("MainSplitter").toByteArray();
  if(!mainSplitterState.isEmpty()){
    ui->mainSplitter->restoreState(mainSplitterState);
  }
  auto horizontalState = m_settings->value("TextHorizontalSplitter").toByteArray();
  if(!horizontalState.isEmpty()){
    ui->textHorizontalSplitter->restoreState(horizontalState);
  }
  auto verticalState = m_settings->value("TextVerticalSplitter").toByteArray();
  if(!verticalState.isEmpty()){
    ui->textVerticalSplitter->restoreState(verticalState);
  }

  m_settings->endGroup();

  // do this outside of settings group because it uses groups internally
  ui->actionAstronomical_data->setChecked (displayAstro);

  m_settings->beginGroup("Common");
  m_mode=m_settings->value("Mode","JT9").toString();
  m_modeTx=m_settings->value("ModeTx","JT9").toString();
  if(m_modeTx.mid(0,3)=="JT9") ui->pbTxMode->setText("Tx JT9  @");
  if(m_modeTx=="JT65") ui->pbTxMode->setText("Tx JT65  #");
  ui->actionNone->setChecked(m_settings->value("SaveNone",true).toBool());
  ui->actionSave_decoded->setChecked(m_settings->value("SaveDecoded",false).toBool());
  ui->actionSave_all->setChecked(m_settings->value("SaveAll",false).toBool());
  ui->RxFreqSpinBox->setValue(0); // ensure a change is signaled
  ui->RxFreqSpinBox->setValue(m_settings->value("RxFreq",1500).toInt());
  m_nSubMode=m_settings->value("SubMode",0).toInt();
  ui->sbFtol->setValue (m_settings->value("Ftol", 20).toInt());
  m_minSync=m_settings->value("MinSync",0).toInt();
  ui->syncSpinBox->setValue(m_minSync);
  ui->cbAutoSeq->setChecked (m_settings->value ("AutoSeq", false).toBool());
  ui->cbRxAll->setChecked (m_settings->value ("RxAll", false).toBool());
  ui->cbVHFcontest->setChecked (m_settings->value ("VHFcontest", false).toBool());
  m_bShMsgs=m_settings->value("ShMsgs",false).toBool();
  m_bSWL=m_settings->value("SWL",false).toBool();
  m_bFast9=m_settings->value("Fast9",false).toBool();
  m_bFastMode=m_settings->value("FastMode",false).toBool();
  ui->sbTR->setValue (m_settings->value ("TRPeriod", 30).toInt());
  m_lastMonitoredFrequency = m_settings->value ("DialFreq",
    QVariant::fromValue<Frequency> (default_frequency)).value<Frequency> ();
  ui->WSPRfreqSpinBox->setValue(0); // ensure a change is signaled
  ui->WSPRfreqSpinBox->setValue(m_settings->value("WSPRfreq",1500).toInt());
  ui->TxFreqSpinBox->setValue(0); // ensure a change is signaled
  ui->TxFreqSpinBox->setValue(m_settings->value("TxFreq",1500).toInt());
  m_ndepth=m_settings->value("NDepth",3).toInt();
  m_pctx=m_settings->value("PctTx",20).toInt();
  m_dBm=m_settings->value("dBm",37).toInt();
  ui->WSPR_prefer_type_1_check_box->setChecked (m_settings->value ("WSPRPreferType1", true).toBool ());
  m_uploadSpots=m_settings->value("UploadSpots",false).toBool();
  if(!m_uploadSpots) ui->cbUploadWSPR_Spots->setStyleSheet("QCheckBox{background-color: yellow}");
  ui->band_hopping_group_box->setChecked (m_settings->value ("BandHopping", false).toBool());
  // setup initial value of tx attenuator
  m_block_pwr_tooltip = true;
  ui->outAttenuation->setValue (m_settings->value ("OutAttenuation", 0).toInt ());
  m_block_pwr_tooltip = false;
  ui->sbCQTxFreq->setValue (m_settings->value ("CQTxFreq", 260).toInt());
  m_noSuffix=m_settings->value("NoSuffix",false).toBool();
  int n=m_settings->value("GUItab",0).toInt();
  ui->tabWidget->setCurrentIndex(n);
  outBufSize=m_settings->value("OutBufSize",4096).toInt();
  ui->cbHoldTxFreq->setChecked (m_settings->value ("HoldTxFreq", false).toBool ());
  m_pwrBandTxMemory=m_settings->value("pwrBandTxMemory").toHash();
  m_pwrBandTuneMemory=m_settings->value("pwrBandTuneMemory").toHash();
  ui->actionEnable_AP_FT8->setChecked (m_settings->value ("FT8AP", false).toBool());
  ui->actionEnable_AP_JT65->setChecked (m_settings->value ("JT65AP", false).toBool());

  m_sortCache = m_settings->value("SortBy").toMap();

  // TODO: jsherer - any other customizations?
  //ui->mainSplitter->setSizes(m_settings->value("MainSplitter", QVariant::fromValue(ui->mainSplitter->sizes())).value<QList<int> >());
  //ui->tableWidgetRXAll->restoreGeometry(m_settings->value("PanelLeftGeometry", ui->tableWidgetRXAll->saveGeometry()).toByteArray());
  //ui->tableWidgetCalls->restoreGeometry(m_settings->value("PanelRightGeometry", ui->tableWidgetCalls->saveGeometry()).toByteArray());
  //ui->extFreeTextMsg->setGeometry( m_settings->value("PanelTopGeometry", ui->extFreeTextMsg->geometry()).toRect());
  //ui->extFreeTextMsgEdit->setGeometry( m_settings->value("PanelBottomGeometry", ui->extFreeTextMsgEdit->geometry()).toRect());
  //ui->bandHorizontalWidget->setGeometry( m_settings->value("PanelWaterfallGeometry", ui->bandHorizontalWidget->geometry()).toRect());
  //qDebug() << m_settings->value("PanelTopGeometry") << ui->extFreeTextMsg;

  ui->textEditRX->setStyleSheet(QString("QTextEdit { background: %1 }").arg(m_config.color_ReceivedMsg().name()));

  {
    auto const& coeffs = m_settings->value ("PhaseEqualizationCoefficients"
                                            , QList<QVariant> {0., 0., 0., 0., 0.}).toList ();
    m_phaseEqCoefficients.clear ();
    for (auto const& coeff : coeffs)
      {
        m_phaseEqCoefficients.append (coeff.value<double> ());
      }
  }
  m_settings->endGroup();

  // use these initialisation settings to tune the audio o/p buffer
  // size and audio thread priority
  m_settings->beginGroup ("Tune");
  m_msAudioOutputBuffered = m_settings->value ("Audio/OutputBufferMs").toInt ();
  m_framesAudioInputBuffered = m_settings->value ("Audio/InputBufferFrames", RX_SAMPLE_RATE / 10).toInt ();
  m_audioThreadPriority = static_cast<QThread::Priority> (m_settings->value ("Audio/ThreadPriority", QThread::HighPriority).toInt () % 8);
  m_settings->endGroup ();

  if (displayMsgAvg) on_actionMessage_averaging_triggered();
}

void MainWindow::set_application_font (QFont const& font)
{
  qApp->setFont (font);
  // set font in the application style sheet as well in case it has
  // been modified in the style sheet which has priority
  qApp->setStyleSheet (qApp->styleSheet () + "* {" + font_as_stylesheet (font) + '}');
  for (auto& widget : qApp->topLevelWidgets ())
    {
      widget->updateGeometry ();
    }
}

void MainWindow::setDecodedTextFont (QFont const& font)
{
  ui->decodedTextBrowser->setContentFont (font);
  ui->decodedTextBrowser2->setContentFont (font);
  ui->textBrowser4->setContentFont(font);
  ui->textBrowser4->displayFoxToBeCalled(" ","#ffffff");
  ui->textBrowser4->setText("");
  auto style_sheet = "QLabel {" + font_as_stylesheet (font) + '}';
  ui->decodedTextLabel->setStyleSheet (ui->decodedTextLabel->styleSheet () + style_sheet);
  ui->decodedTextLabel2->setStyleSheet (ui->decodedTextLabel2->styleSheet () + style_sheet);
  if (m_msgAvgWidget) {
    m_msgAvgWidget->changeFont (font);
  }
  updateGeometry ();
}

void MainWindow::fixStop()
{
  m_hsymStop=179;
  if(m_mode=="WSPR") {
    m_hsymStop=396;
  } else if(m_mode=="WSPR-LF") {
    m_hsymStop=813;
  } else if(m_mode=="Echo") {
    m_hsymStop=9;
  } else if (m_mode=="JT4"){
    m_hsymStop=176;
    if(m_config.decode_at_52s()) m_hsymStop=179;
  } else if (m_mode=="JT9"){
    m_hsymStop=173;
    if(m_config.decode_at_52s()) m_hsymStop=179;
  } else if (m_mode=="JT65" or m_mode=="JT9+JT65"){
    m_hsymStop=174;
    if(m_config.decode_at_52s()) m_hsymStop=179;
  } else if (m_mode=="QRA64"){
    m_hsymStop=179;
    if(m_config.decode_at_52s()) m_hsymStop=186;
  } else if (m_mode=="FreqCal"){
    m_hsymStop=((int(m_TRperiod/0.288))/8)*8;
  } else if (m_mode=="FT8") {
    m_hsymStop=50;
  }
}

//-------------------------------------------------------------- dataSink()
void MainWindow::dataSink(qint64 frames)
{
  static float s[NSMAX];
  char line[80];

  int k (frames);
  QString fname {QDir::toNativeSeparators(m_config.writeable_data_dir ().absoluteFilePath ("refspec.dat"))};
  QByteArray bafname = fname.toLatin1();
  const char *c_fname = bafname.data();
  int len=fname.length();

  if(m_diskData) {
    dec_data.params.ndiskdat=1;
  } else {
    dec_data.params.ndiskdat=0;
  }

  m_bUseRef=m_wideGraph->useRef();
  refspectrum_(&dec_data.d2[k-m_nsps/2],&m_bClearRefSpec,&m_bRefSpec,
      &m_bUseRef,c_fname,len);
  m_bClearRefSpec=false;

  if(m_mode=="ISCAT" or m_mode=="MSK144" or m_bFast9) {
    fastSink(frames);
    if(m_bFastMode) return;
  }

// Get power, spectrum, and ihsym
  int trmin=m_TRperiod/60;
//  int k (frames - 1);
  dec_data.params.nfa=m_wideGraph->nStartFreq();
  dec_data.params.nfb=m_wideGraph->Fmax();
  int nsps=m_nsps;
  if(m_bFastMode) nsps=6912;
  int nsmo=m_wideGraph->smoothYellow()-1;
  symspec_(&dec_data,&k,&trmin,&nsps,&m_inGain,&nsmo,&m_px,s,&m_df3,&m_ihsym,&m_npts8,&m_pxmax);
  if(m_mode=="WSPR") wspr_downsample_(dec_data.d2,&k);
  if(m_ihsym <=0) return;
  if(ui) ui->signal_meter_widget->setValue(m_px,m_pxmax); // Update thermometer
  if(m_monitoring || m_diskData) {
    m_wideGraph->dataSink2(s,m_df3,m_ihsym,m_diskData);
  }
  if(m_mode=="MSK144") return;

  fixStop();
  if (m_mode == "FreqCal"
      // only calculate after 1st chunk, also skip chunk where rig
      // changed frequency
      && !(m_ihsym % 8) && m_ihsym > 8 && m_ihsym <= m_hsymStop) {
    int RxFreq=ui->RxFreqSpinBox->value ();
    int nkhz=(m_freqNominal+RxFreq)/1000;
    int ftol = ui->sbFtol->value ();
    freqcal_(&dec_data.d2[0],&k,&nkhz,&RxFreq,&ftol,&line[0],80);
    QString t=QString::fromLatin1(line);
    DecodedText decodedtext {t, false, m_config.my_grid ()};
    ui->decodedTextBrowser->displayDecodedText (decodedtext,m_baseCall,m_config.DXCC(),
                                                m_logBook,m_config.color_CQ(),m_config.color_MyCall(),m_config.color_DXCC(),
                                                m_config.color_NewCall(),m_config.ppfx());
    if (ui->measure_check_box->isChecked ()) {
      // Append results text to file "fmt.all".
      QFile f {m_config.writeable_data_dir ().absoluteFilePath ("fmt.all")};
      if (f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
        QTextStream out(&f);
        out << t << endl;
        f.close();
      } else {
        MessageBox::warning_message (this, tr ("File Open Error")
                                     , tr ("Cannot open \"%1\" for append: %2")
                                     .arg (f.fileName ()).arg (f.errorString ()));
      }
    }
    if(m_ihsym==m_hsymStop && ui->actionFrequency_calibration->isChecked()) {
      freqCalStep();
    }
  }

  if(m_ihsym==3*m_hsymStop/4) {
    m_dialFreqRxWSPR=m_freqNominal;
  }

  if(m_ihsym == m_hsymStop) {
    if(m_mode=="Echo") {
      float snr=0;
      int nfrit=0;
      int nqual=0;
      float f1=1500.0;
      float xlevel=0.0;
      float sigdb=0.0;
      float dfreq=0.0;
      float width=0.0;
      echocom_.nclearave=m_nclearave;
      int nDop=0;
      avecho_(dec_data.d2,&nDop,&nfrit,&nqual,&f1,&xlevel,&sigdb,
          &snr,&dfreq,&width);
      QString t;
      t.sprintf("%3d %7.1f %7.1f %7.1f %7.1f %3d",echocom_.nsum,xlevel,sigdb,
                dfreq,width,nqual);
      t=QDateTime::currentDateTimeUtc().toString("hh:mm:ss  ") + t;
      if (ui) ui->decodedTextBrowser->appendText(t);
      if(m_echoGraph->isVisible()) m_echoGraph->plotSpec();
      m_nclearave=0;
//Don't restart Monitor after an Echo transmission
      if(m_bEchoTxed and !m_auto) {
        monitor(false);
        m_bEchoTxed=false;
      }
      return;
    }
    if(m_mode=="FreqCal") {
      return;
    }
    if( m_dialFreqRxWSPR==0) m_dialFreqRxWSPR=m_freqNominal;
    m_dataAvailable=true;
    dec_data.params.npts8=(m_ihsym*m_nsps)/16;
    dec_data.params.newdat=1;
    dec_data.params.nagain=0;
    dec_data.params.nzhsym=m_hsymStop;
    QDateTime now {QDateTime::currentDateTimeUtc ()};
    m_dateTime = now.toString ("yyyy-MMM-dd hh:mm");
    if(!m_mode.startsWith ("WSPR")) decode(); //Start decoder

    if(!m_diskData) {                        //Always save; may delete later

      if(m_mode=="FT8") {
        int n=now.time().second() % m_TRperiod;
        if(n<(m_TRperiod/2)) n=n+m_TRperiod;
        auto const& period_start=now.addSecs(-n);
        m_fnameWE=m_config.save_directory().absoluteFilePath (period_start.toString("yyMMdd_hhmmss"));
      } else {
        auto const& period_start = now.addSecs (-(now.time ().minute () % (m_TRperiod / 60)) * 60);
        m_fnameWE=m_config.save_directory ().absoluteFilePath (period_start.toString ("yyMMdd_hhmm"));
      }
      m_fileToSave.clear ();

      // the following is potential a threading hazard - not a good
      // idea to pass pointer to be processed in another thread
      m_saveWAVWatcher.setFuture (QtConcurrent::run (std::bind (&MainWindow::save_wave_file,
            this, m_fnameWE, &dec_data.d2[0], m_TRperiod, m_config.my_callsign(),
            m_config.my_grid(), m_mode, m_nSubMode, m_freqNominal, m_hisCall, m_hisGrid)));
      if (m_mode=="WSPR") {
        QString c2name_string {m_fnameWE + ".c2"};
        int len1=c2name_string.length();
        char c2name[80];
        strcpy(c2name,c2name_string.toLatin1 ().constData ());
        int nsec=120;
        int nbfo=1500;
        double f0m1500=m_freqNominal/1000000.0 + nbfo - 1500;
        int err = savec2_(c2name,&nsec,&f0m1500,len1);
        if (err!=0) MessageBox::warning_message (this, tr ("Error saving c2 file"), c2name);
      }
    }

    if(m_mode.startsWith ("WSPR")) {
      QString t2,cmnd,depth_string;
      double f0m1500=m_dialFreqRxWSPR/1000000.0;   // + 0.000001*(m_BFO - 1500);
      t2.sprintf(" -f %.6f ",f0m1500);
      if((m_ndepth&7)==1) depth_string=" -qB "; //2 pass w subtract, no Block detection, no shift jittering
      if((m_ndepth&7)==2) depth_string=" -B ";  //2 pass w subtract, no Block detection
      if((m_ndepth&7)==3) depth_string=" ";     //2 pass w subtract, Block detection
      QString degrade;
      degrade.sprintf("-d %4.1f ",m_config.degrade());

      if(m_diskData) {
        cmnd='"' + m_appDir + '"' + "/wsprd " + depth_string + " -a \"" +
          QDir::toNativeSeparators(m_config.writeable_data_dir ().absolutePath()) + "\" \"" + m_path + "\"";
      } else {
        if(m_mode=="WSPR-LF") {
          cmnd='"' + m_appDir + '"' + "/wspr_fsk8d " + degrade + t2 +" -a \"" +
            QDir::toNativeSeparators(m_config.writeable_data_dir ().absolutePath()) + "\" " +
              '"' + m_fnameWE + ".wav\"";
        } else {
          cmnd='"' + m_appDir + '"' + "/wsprd " + depth_string + " -a \"" +
            QDir::toNativeSeparators(m_config.writeable_data_dir ().absolutePath()) + "\" " +
              t2 + '"' + m_fnameWE + ".wav\"";
        }
      }
      QString t3=cmnd;
      int i1=cmnd.indexOf("/wsprd ");
      cmnd=t3.mid(0,i1+7) + t3.mid(i1+7);

      if(m_mode=="WSPR-LF") cmnd=cmnd.replace("/wsprd ","/wspr_fsk8d "+degrade+t2);
      if (ui) ui->DecodeButton->setChecked (true);
      m_cmndP1=QDir::toNativeSeparators(cmnd);
      p1Timer.start(1000);
      m_decoderBusy = true;
      statusUpdate ();
    }
    m_rxDone=true;
  }
}

void MainWindow::startP1()
{
  p1.start(m_cmndP1);
}

QString MainWindow::save_wave_file (QString const& name, short const * data, int seconds,
        QString const& my_callsign, QString const& my_grid, QString const& mode, qint32 sub_mode,
        Frequency frequency, QString const& his_call, QString const& his_grid) const
{
  //
  // This member function runs in a thread and should not access
  // members that may be changed in the GUI thread or any other thread
  // without suitable synchronization.
  //
  QAudioFormat format;
  format.setCodec ("audio/pcm");
  format.setSampleRate (12000);
  format.setChannelCount (1);
  format.setSampleSize (16);
  format.setSampleType (QAudioFormat::SignedInt);
  auto source = QString {"%1, %2"}.arg (my_callsign).arg (my_grid);
  auto comment = QString {"Mode=%1%2, Freq=%3%4"}
     .arg (mode)
     .arg (QString {mode.contains ('J') && !mode.contains ('+')
           ? QString {", Sub Mode="} + QChar {'A' + sub_mode}
         : QString {}})
        .arg (Radio::frequency_MHz_string (frequency))
     .arg (QString {!mode.startsWith ("WSPR") ? QString {", DXCall=%1, DXGrid=%2"}
         .arg (his_call)
         .arg (his_grid).toLocal8Bit () : ""});
  BWFFile::InfoDictionary list_info {
      {{{'I','S','R','C'}}, source.toLocal8Bit ()},
      {{{'I','S','F','T'}}, program_title (revision ()).simplified ().toLocal8Bit ()},
      {{{'I','C','R','D'}}, QDateTime::currentDateTime ()
                          .toString ("yyyy-MM-ddTHH:mm:ss.zzzZ").toLocal8Bit ()},
      {{{'I','C','M','T'}}, comment.toLocal8Bit ()},
        };
  auto file_name = name + ".wav";
  BWFFile wav {format, file_name, list_info};
  if (!wav.open (BWFFile::WriteOnly)
      || 0 > wav.write (reinterpret_cast<char const *> (data)
                        , sizeof (short) * seconds * format.sampleRate ()))
    {
      return file_name + ": " + wav.errorString ();
    }
  return QString {};
}

//-------------------------------------------------------------- fastSink()
void MainWindow::fastSink(qint64 frames)
{
  int k (frames);
  bool decodeNow=false;

  if(k < m_k0) {                                 //New sequence ?
    memcpy(fast_green2,fast_green,4*703);        //Copy fast_green[] to fast_green2[]
    memcpy(fast_s2,fast_s,4*703*64);             //Copy fast_s[] into fast_s2[]
    fast_jh2=fast_jh;
    if(!m_diskData) memset(dec_data.d2,0,2*30*12000);   //Zero the d2[] array
    m_bFastDecodeCalled=false;
    m_bDecoded=false;
  }

  QDateTime tnow=QDateTime::currentDateTimeUtc();
  int ihr=tnow.toString("hh").toInt();
  int imin=tnow.toString("mm").toInt();
  int isec=tnow.toString("ss").toInt();
  isec=isec - isec%m_TRperiod;
  int nutc0=10000*ihr + 100*imin + isec;
  if(m_diskData) nutc0=m_UTCdisk;
  char line[80];
  bool bmsk144=((m_mode=="MSK144") and (m_monitoring or m_diskData));
  line[0]=0;

  int RxFreq=ui->RxFreqSpinBox->value ();
  int nTRpDepth=m_TRperiod + 1000*(m_ndepth & 3);
  qint64 ms0 = QDateTime::currentMSecsSinceEpoch();
  strncpy(dec_data.params.mycall, (m_baseCall+"            ").toLatin1(),12);
  QString hisCall {ui->dxCallEntry->text ()};
  bool bshmsg=ui->cbShMsgs->isChecked();
  bool bcontest=ui->cbVHFcontest->isChecked();
  bool bswl=ui->cbSWL->isChecked();
  strncpy(dec_data.params.hiscall,(Radio::base_callsign (hisCall) + "            ").toLatin1 ().constData (), 12);
  strncpy(dec_data.params.mygrid, (m_config.my_grid()+"      ").toLatin1(),6);
  QString dataDir;
  dataDir = m_config.writeable_data_dir ().absolutePath ();
  char ddir[512];
  strncpy(ddir,dataDir.toLatin1(), sizeof (ddir) - 1);
  float pxmax = 0;
  float rmsNoGain = 0;
  int ftol = ui->sbFtol->value ();
  hspec_(dec_data.d2,&k,&nutc0,&nTRpDepth,&RxFreq,&ftol,&bmsk144,&bcontest,
         &m_bTrain,m_phaseEqCoefficients.constData(),&m_inGain,&dec_data.params.mycall[0],
         &dec_data.params.hiscall[0],&bshmsg,&bswl,
         &ddir[0],fast_green,fast_s,&fast_jh,&pxmax,&rmsNoGain,&line[0],&dec_data.params.mygrid[0],
         12,12,512,80,6);
  float px = fast_green[fast_jh];
  QString t;
  t.sprintf(" Rx noise: %5.1f ",px);
  ui->signal_meter_widget->setValue(rmsNoGain,pxmax); // Update thermometer
  m_fastGraph->plotSpec(m_diskData,m_UTCdisk);

  if(bmsk144 and (line[0]!=0)) {
    QString message {QString::fromLatin1 (line)};
    DecodedText decodedtext {message.replace (QChar::LineFeed, ""), bcontest, m_config.my_grid ()};
    ui->decodedTextBrowser->displayDecodedText (decodedtext,m_baseCall,m_config.DXCC(),
         m_logBook,m_config.color_CQ(),m_config.color_MyCall(),m_config.color_DXCC(),
         m_config.color_NewCall(),m_config.ppfx());
    m_bDecoded=true;
    auto_sequence (decodedtext, ui->sbFtol->value (), std::numeric_limits<unsigned>::max ());
    if (m_mode != "ISCAT") postDecode (true, decodedtext.string ());
    writeAllTxt(message);
    bool stdMsg = decodedtext.report(m_baseCall,
                  Radio::base_callsign(ui->dxCallEntry->text()),m_rptRcvd);
    //if (stdMsg) pskPost (decodedtext);
  }

  float fracTR=float(k)/(12000.0*m_TRperiod);
  decodeNow=false;
  if(fracTR>0.92) {
    m_dataAvailable=true;
    fast_decode_done();
    m_bFastDone=true;
  }

  m_k0=k;
  if(m_diskData and m_k0 >= dec_data.params.kin - 7 * 512) decodeNow=true;
  if(!m_diskData and m_tRemaining<0.35 and !m_bFastDecodeCalled) decodeNow=true;
  if(m_mode=="MSK144") decodeNow=false;

  if(decodeNow) {
    m_dataAvailable=true;
    m_t0=0.0;
    m_t1=k/12000.0;
    m_kdone=k;
    dec_data.params.newdat=1;
    if(!m_decoderBusy) {
      m_bFastDecodeCalled=true;
      decode();
    }
  }

  if(decodeNow or m_bFastDone) {
    if(!m_diskData) {
      QDateTime now {QDateTime::currentDateTimeUtc()};
      int n=now.time().second() % m_TRperiod;
      if(n<(m_TRperiod/2)) n=n+m_TRperiod;
      auto const& period_start = now.addSecs (-n);
      m_fnameWE = m_config.save_directory ().absoluteFilePath (period_start.toString ("yyMMdd_hhmmss"));
      m_fileToSave.clear ();
      if(m_saveAll or m_bAltV or (m_bDecoded and m_saveDecoded) or (m_mode!="MSK144")) {
        m_bAltV=false;
        // the following is potential a threading hazard - not a good
        // idea to pass pointer to be processed in another thread
        m_saveWAVWatcher.setFuture (QtConcurrent::run (std::bind (&MainWindow::save_wave_file,
           this, m_fnameWE, &dec_data.d2[0], m_TRperiod, m_config.my_callsign(),
           m_config.my_grid(), m_mode, m_nSubMode, m_freqNominal, m_hisCall, m_hisGrid)));
      }
      if(m_mode!="MSK144") {
        killFileTimer.start (3*1000*m_TRperiod/4); //Kill 3/4 period from now
      }
    }
    m_bFastDone=false;
  }
  float tsec=0.001*(QDateTime::currentMSecsSinceEpoch() - ms0);
  m_fCPUmskrtd=0.9*m_fCPUmskrtd + 0.1*tsec;
}

void MainWindow::showSoundInError(const QString& errorMsg)
{
  if (m_splash && m_splash->isVisible ()) m_splash->hide ();
  MessageBox::critical_message (this, tr ("Error in Sound Input"), errorMsg);
}

void MainWindow::showSoundOutError(const QString& errorMsg)
{
  if (m_splash && m_splash->isVisible ()) m_splash->hide ();
  MessageBox::critical_message (this, tr ("Error in Sound Output"), errorMsg);
}

void MainWindow::showStatusMessage(const QString& statusMsg)
{statusBar()->showMessage(statusMsg);}


void MainWindow::on_menuWindow_aboutToShow(){
    auto hsizes = ui->textHorizontalSplitter->sizes();
    ui->actionShow_Band_Activity->setChecked(hsizes.at(0) > 0);
    ui->actionShow_Call_Activity->setChecked(hsizes.at(2) > 0);

    auto vsizes = ui->mainSplitter->sizes();
    ui->actionShow_Waterfall->setChecked(vsizes.last() > 0);

    QMenu * sortBandMenu = new QMenu(ui->menuWindow);
    buildBandActivitySortByMenu(sortBandMenu);
    ui->actionSort_Band_Activity->setMenu(sortBandMenu);

    QMenu * sortCallMenu = new QMenu(ui->menuWindow);
    buildCallActivitySortByMenu(sortCallMenu);
    ui->actionSort_Call_Activity->setMenu(sortCallMenu);
}

void MainWindow::on_actionShow_Band_Activity_triggered(bool checked){
    auto hsizes = ui->textHorizontalSplitter->sizes();
    hsizes[0] = checked ? ui->textHorizontalSplitter->width()/4 : 0;
    ui->textHorizontalSplitter->setSizes(hsizes);
}

void MainWindow::on_actionShow_Call_Activity_triggered(bool checked){
    auto hsizes = ui->textHorizontalSplitter->sizes();
    hsizes[2] = checked ? ui->textHorizontalSplitter->width()/4 : 0;
    ui->textHorizontalSplitter->setSizes(hsizes);
}

void MainWindow::on_actionShow_Waterfall_triggered(bool checked){
    auto vsizes = ui->mainSplitter->sizes();
    vsizes[0] = qMin(vsizes[0], ui->logHorizontalWidget->minimumHeight());
    int oldHeight = vsizes[vsizes.length()-1];
    int newHeight = checked ? ui->mainSplitter->height()/4 : 0;
    vsizes[1] += oldHeight - newHeight;
    vsizes[vsizes.length()-1] = newHeight;
    ui->mainSplitter->setSizes(vsizes);
}

void MainWindow::on_actionReset_Window_Sizes_triggered(){
    auto size = this->centralWidget()->size();

    ui->mainSplitter->setSizes({
        ui->logHorizontalWidget->minimumHeight(),
        ui->mainSplitter->height()/2,
        ui->macroHorizonalWidget->minimumHeight(),
        ui->mainSplitter->height()/4
    });

    ui->textHorizontalSplitter->setSizes({
        ui->textHorizontalSplitter->width()/4,
        ui->textHorizontalSplitter->width()/2,
        ui->textHorizontalSplitter->width()/4
    });

    ui->textVerticalSplitter->setSizes({
        ui->textVerticalSplitter->height()/2,
        ui->textVerticalSplitter->height()/2
    });
}

void MainWindow::on_actionSettings_triggered()               //Setup Dialog
{
  // things that might change that we need know about
  auto callsign = m_config.my_callsign ();
  auto my_grid = m_config.my_grid ();
  if (QDialog::Accepted == m_config.exec ()) {
    if (m_config.my_callsign () != callsign) {
      m_baseCall = Radio::base_callsign (m_config.my_callsign ());
      morse_(const_cast<char *> (m_config.my_callsign ().toLatin1().constData()),
             const_cast<int *> (icw), &m_ncw, m_config.my_callsign ().length());
    }
    if (m_config.my_callsign () != callsign || m_config.my_grid () != my_grid) {
      statusUpdate ();
    }
    on_dxGridEntry_textChanged (m_hisGrid); // recalculate distances in case of units change
    enable_DXCC_entity (m_config.DXCC ());  // sets text window proportions and (re)inits the logbook

    prepareSpotting();

    if(m_config.restart_audio_input ()) {
      Q_EMIT startAudioInputStream (m_config.audio_input_device (),
                 m_framesAudioInputBuffered, m_detector, m_downSampleFactor,
                                      m_config.audio_input_channel ());
    }

    if(m_config.restart_audio_output ()) {
      Q_EMIT initializeAudioOutputStream (m_config.audio_output_device (),
           AudioDevice::Mono == m_config.audio_output_channel () ? 1 : 2,
                                          m_msAudioOutputBuffered);
    }

    displayDialFrequency ();
    displayActivity(true);

    bool vhf {m_config.enable_VHF_features()};
    m_wideGraph->setVHF(vhf);
    if (!vhf) ui->sbSubmode->setValue (0);

    setup_status_bar (vhf);
    bool b = vhf && (m_mode=="JT4" or m_mode=="JT65" or m_mode=="ISCAT" or
                     m_mode=="JT9" or m_mode=="MSK144" or m_mode=="QRA64");
    if(b) VHF_features_enabled(b);
    if(m_mode=="FT8") on_actionFT8_triggered();
    if(b) VHF_features_enabled(b);

    m_config.transceiver_online ();
    if(!m_bFastMode) setXIT (ui->TxFreqSpinBox->value ());
    if(m_config.single_decode() or m_mode=="JT4") {
      ui->label_6->setText("Single-Period Decodes");
      ui->label_7->setText("Average Decodes");
    } else {
//      ui->label_6->setText("Band Activity");
//      ui->label_7->setText("Rx Frequency");
    }
    update_watchdog_label ();
    if(!m_splitMode) ui->cbCQTx->setChecked(false);
    if(!m_config.enable_VHF_features()) {
      ui->actionInclude_averaging->setVisible(false);
      ui->actionInclude_correlation->setVisible (false);
      ui->actionInclude_averaging->setChecked(false);
      ui->actionInclude_correlation->setChecked(false);
      ui->actionEnable_AP_JT65->setVisible(false);
    }
    m_opCall=m_config.opCall();
  }
}

void MainWindow::prepareSpotting(){
    if(m_config.spot_to_reporting_networks ()){
        pskSetLocal();
        aprsSetLocal();
        ui->spotButton->setChecked(true);
    } else {
        ui->spotButton->setChecked(false);
    }
}

void MainWindow::on_spotButton_clicked(bool checked){
    // 1. save setting
    m_config.set_spot_to_reporting_networks(checked);

    // 2. prepare
    prepareSpotting();
}

void MainWindow::on_monitorButton_clicked (bool checked)
{
  if (!m_transmitting) {
    auto prior = m_monitoring;
    monitor (checked);
    if (checked && !prior) {
      if (m_config.monitor_last_used ()) {
              // put rig back where it was when last in control
        setRig (m_lastMonitoredFrequency);
        setXIT (ui->TxFreqSpinBox->value ());
      }
          // ensure FreqCal triggers
      on_RxFreqSpinBox_valueChanged (ui->RxFreqSpinBox->value ());
    }
      //Get Configuration in/out of strict split and mode checking
    Q_EMIT m_config.sync_transceiver (true, checked);
  } else {
    ui->monitorButton->setChecked (false); // disallow
  }
}

void MainWindow::monitor (bool state)
{
  ui->monitorButton->setChecked (state);
  if (state) {
    m_diskData = false; // no longer reading WAV files
    if (!m_monitoring) Q_EMIT resumeAudioInputStream ();
  } else {
    Q_EMIT suspendAudioInputStream ();
  }
  m_monitoring = state;
}

void MainWindow::on_actionAbout_triggered()                  //Display "About"
{
  CAboutDlg {this}.exec ();
}

void MainWindow::on_autoButton_clicked (bool checked)
{
  m_auto = checked;
  if (checked
      && ui->cbFirst->isVisible () && ui->cbFirst->isChecked()
      && CALLING == m_QSOProgress) {
    m_bAutoReply = false;         // ready for next
    m_bCallingCQ = true;        // allows tail-enders to be picked up
    ui->cbFirst->setStyleSheet ("QCheckBox{color:red}");
  } else {
    ui->cbFirst->setStyleSheet("");
  }
  if (!checked) m_bCallingCQ = false;
  statusUpdate ();
  m_bEchoTxOK=false;
  if(m_auto and (m_mode=="Echo")) {
    m_nclearave=1;
    echocom_.nsum=0;
  }
  if(m_mode.startsWith ("WSPR"))  {
    QPalette palette {ui->sbTxPercent->palette ()};
    if(m_auto or m_pctx==0) {
      palette.setColor(QPalette::Base,Qt::white);
    } else {
      palette.setColor(QPalette::Base,Qt::yellow);
    }
    ui->sbTxPercent->setPalette(palette);
  }
  m_tAutoOn=QDateTime::currentMSecsSinceEpoch()/1000;

  // stop tx, reset the ui and message queue
  if(!checked){
      on_stopTxButton_clicked();
  }
}

void MainWindow::auto_tx_mode (bool state)
{
    ui->autoButton->setChecked (state);
    on_autoButton_clicked (state);
}

void MainWindow::keyPressEvent (QKeyEvent * e)
{
    switch (e->key()) {
        case Qt::Key_Escape:
            on_stopTxButton_clicked();
            stopTx();
            return;
    }

    QMainWindow::keyPressEvent (e);
}

void MainWindow::bumpFqso(int n)                                 //bumpFqso()
{
  int i;
  bool ctrl = (n>=100);
  n=n%100;
  i=ui->RxFreqSpinBox->value();
  bool bTrackTx=ui->TxFreqSpinBox->value() == i;
  if(n==11) i--;
  if(n==12) i++;
  if (ui->RxFreqSpinBox->isEnabled ()) {
    ui->RxFreqSpinBox->setValue (i);
  }
  if(ctrl and m_mode.startsWith ("WSPR")) {
    ui->WSPRfreqSpinBox->setValue(i);
  } else {
    if(ctrl and bTrackTx) {
      ui->TxFreqSpinBox->setValue (i);
    }
  }
}

Radio::Frequency MainWindow::dialFrequency() {
    return Frequency {m_rigState.ptt () && m_rigState.split () ?
        m_rigState.tx_frequency () : m_rigState.frequency ()};
}

void MainWindow::displayDialFrequency ()
{
  auto dial_frequency = dialFrequency();

  // lookup band
  auto const& band_name = m_config.bands ()->find (dial_frequency);
  if (m_lastBand != band_name){
      cacheActivity(m_lastBand);

      // only change this when necessary as we get called a lot and it
      // would trash any user input to the band combo box line edit
      ui->bandComboBox->setCurrentText (band_name);
      m_wideGraph->setRxBand (band_name);
      m_lastBand = band_name;
      band_changed(dial_frequency);

      clearActivity();

      restoreActivity(m_lastBand);
  }

  // TODO: jsherer - this doesn't validate anything else right? we are disabling this because as long as you're in a band, it's valid.
  /*
  // search working frequencies for one we are within 10kHz of (1 Mhz
  // of on VHF and up)
  bool valid {false};
  quint64 min_offset {99999999};
  for (auto const& item : *m_config.frequencies ())
    {
      // we need to do specific checks for above and below here to
      // ensure that we can use unsigned Radio::Frequency since we
      // potentially use the full 64-bit unsigned range.
      auto const& working_frequency = item.frequency_;
      auto const& offset = dial_frequency > working_frequency ?
        dial_frequency - working_frequency :
        working_frequency - dial_frequency;
      if (offset < min_offset) {
        min_offset = offset;
      }
    }
  if (min_offset < 10000u || (m_config.enable_VHF_features() && min_offset < 1000000u)) {
    valid = true;
  }
  */

  bool valid = !band_name.isEmpty();

  update_dynamic_property (ui->labDialFreq, "oob", !valid);
  ui->labDialFreq->setText (Radio::pretty_frequency_MHz_string (dial_frequency));
}

void MainWindow::statusChanged()
{
  statusUpdate ();
  QFile f {m_config.temp_dir ().absoluteFilePath ("wsjtx_status.txt")};
  if(f.open(QFile::WriteOnly | QIODevice::Text)) {
    QTextStream out(&f);
    QString tmpGrid = m_hisGrid;
    if (!tmpGrid.size ()) tmpGrid="n/a"; // Not Available
    out << qSetRealNumberPrecision (12) << (m_freqNominal / 1.e6)
        << ";" << m_mode << ";" << m_hisCall << ";"
        << ui->rptSpinBox->value() << ";" << m_modeTx << ";" << tmpGrid << endl;
    f.close();
  } else {
    if (m_splash && m_splash->isVisible ()) m_splash->hide ();
    MessageBox::warning_message (this, tr ("Status File Error")
                                 , tr ("Cannot open \"%1\" for writing: %2")
                                 .arg (f.fileName ()).arg (f.errorString ()));
  }
  on_dxGridEntry_textChanged(m_hisGrid);
}

bool MainWindow::eventFilter (QObject * object, QEvent * event)
{
  switch (event->type())
    {
    case QEvent::KeyPress:
      // fall through
    case QEvent::MouseButtonPress:
      // reset the Tx watchdog
      tx_watchdog (false);
      break;

    case QEvent::ChildAdded:
      // ensure our child widgets get added to our event filter
      add_child_to_event_filter (static_cast<QChildEvent *> (event)->child ());
      break;

    case QEvent::ChildRemoved:
      // ensure our child widgets get d=removed from our event filter
      remove_child_from_event_filter (static_cast<QChildEvent *> (event)->child ());
      break;

    default: break;
    }
  return QObject::eventFilter(object, event);
}

void MainWindow::createStatusBar()                           //createStatusBar
{
  tx_status_label.setAlignment (Qt::AlignHCenter);
  tx_status_label.setMinimumSize (QSize  {150, 18});
  tx_status_label.setStyleSheet ("QLabel{background-color: #00ff00}");
  tx_status_label.setFrameStyle (QFrame::Panel | QFrame::Sunken);
  statusBar()->addWidget (&tx_status_label);

  config_label.setAlignment (Qt::AlignHCenter);
  config_label.setMinimumSize (QSize {80, 18});
  config_label.setFrameStyle (QFrame::Panel | QFrame::Sunken);
  statusBar()->addWidget (&config_label);
  config_label.hide ();         // only shown for non-default configuration

  mode_label.setAlignment (Qt::AlignHCenter);
  mode_label.setMinimumSize (QSize {80, 18});
  mode_label.setFrameStyle (QFrame::Panel | QFrame::Sunken);
  statusBar()->addWidget (&mode_label);

  last_tx_label.setAlignment (Qt::AlignHCenter);
  last_tx_label.setMinimumSize (QSize {150, 18});
  last_tx_label.setFrameStyle (QFrame::Panel | QFrame::Sunken);
  statusBar()->addWidget (&last_tx_label);

  band_hopping_label.setAlignment (Qt::AlignHCenter);
  band_hopping_label.setMinimumSize (QSize {90, 18});
  band_hopping_label.setFrameStyle (QFrame::Panel | QFrame::Sunken);

  statusBar()->addPermanentWidget(&progressBar, 1);
  progressBar.setMinimumSize (QSize {100, 18});
  progressBar.setFormat ("%v/%m");

  statusBar ()->addPermanentWidget (&watchdog_label);
  update_watchdog_label ();
}

void MainWindow::setup_status_bar (bool vhf)
{
  auto submode = current_submode ();
  if (vhf && submode != QChar::Null)
    {
      mode_label.setText (m_mode + " " + submode);
    }
  else
    {
      if(m_mode == "FT8"){
        mode_label.setText("FT8CALL");
      } else {
        mode_label.setText (m_mode);
      }
    }
  if ("ISCAT" == m_mode) {
    mode_label.setStyleSheet ("QLabel{background-color: #ff9933}");
  } else if ("JT9" == m_mode) {
    mode_label.setStyleSheet ("QLabel{background-color: #ff6ec7}");
  } else if ("JT4" == m_mode) {
    mode_label.setStyleSheet ("QLabel{background-color: #cc99ff}");
  } else if ("Echo" == m_mode) {
    mode_label.setStyleSheet ("QLabel{background-color: #66ffff}");
  } else if ("JT9+JT65" == m_mode) {
    mode_label.setStyleSheet ("QLabel{background-color: #ffff66}");
  } else if ("JT65" == m_mode) {
    mode_label.setStyleSheet ("QLabel{background-color: #66ff66}");
  } else if ("QRA64" == m_mode) {
    mode_label.setStyleSheet ("QLabel{background-color: #99ff33}");
  } else if ("MSK144" == m_mode) {
    mode_label.setStyleSheet ("QLabel{background-color: #ff6666}");
  } else if ("FT8" == m_mode) {
    mode_label.setStyleSheet ("QLabel{background-color: #6699ff}");
  } else if ("FreqCal" == m_mode) {
    mode_label.setStyleSheet ("QLabel{background-color: #ff9933}");  }
  last_tx_label.setText (QString {});
  if (m_mode.contains (QRegularExpression {R"(^(Echo|ISCAT))"})) {
    if (band_hopping_label.isVisible ()) statusBar ()->removeWidget (&band_hopping_label);
  } else if (m_mode.startsWith ("WSPR")) {
    mode_label.setStyleSheet ("QLabel{background-color: #ff66ff}");
    if (!band_hopping_label.isVisible ()) {
      statusBar ()->addWidget (&band_hopping_label);
      band_hopping_label.show ();
    }
  } else {
    if (band_hopping_label.isVisible ()) statusBar ()->removeWidget (&band_hopping_label);
  }
}

void MainWindow::subProcessFailed (QProcess * process, int exit_code, QProcess::ExitStatus status)
{
  if (m_valid && (exit_code || QProcess::NormalExit != status))
    {
      QStringList arguments;
      for (auto argument: process->arguments ())
        {
          if (argument.contains (' ')) argument = '"' + argument + '"';
          arguments << argument;
        }
      if (m_splash && m_splash->isVisible ()) m_splash->hide ();
      MessageBox::critical_message (this, tr ("Subprocess Error")
                                    , tr ("Subprocess failed with exit code %1")
                                    .arg (exit_code)
                                    , tr ("Running: %1\n%2")
                                    .arg (process->program () + ' ' + arguments.join (' '))
                                    .arg (QString {process->readAllStandardError()}));
      QTimer::singleShot (0, this, SLOT (close ()));
      m_valid = false;          // ensures exit if still constructing
    }
}

void MainWindow::subProcessError (QProcess * process, QProcess::ProcessError)
{
  if (m_valid)
    {
      QStringList arguments;
      for (auto argument: process->arguments ())
        {
          if (argument.contains (' ')) argument = '"' + argument + '"';
          arguments << argument;
        }
      if (m_splash && m_splash->isVisible ()) m_splash->hide ();
      MessageBox::critical_message (this, tr ("Subprocess error")
                                    , tr ("Running: %1\n%2")
                                    .arg (process->program () + ' ' + arguments.join (' '))
                                    .arg (process->errorString ()));
      QTimer::singleShot (0, this, SLOT (close ()));
      m_valid = false;              // ensures exit if still constructing
    }
}

void MainWindow::closeEvent(QCloseEvent * e)
{
  m_valid = false;              // suppresses subprocess errors
  m_config.transceiver_offline ();
  writeSettings ();
  m_astroWidget.reset ();
  m_guiTimer.stop ();
  m_prefixes.reset ();
  m_shortcuts.reset ();
  m_mouseCmnds.reset ();
  if(m_mode!="MSK144" and m_mode!="FT8") killFile();
  float sw=0.0;
  int nw=400;
  int nh=100;
  int irow=-99;
  plotsave_(&sw,&nw,&nh,&irow);
  mem_jt9->detach();
  QFile quitFile {m_config.temp_dir ().absoluteFilePath (".quit")};
  quitFile.open(QIODevice::ReadWrite);
  QFile {m_config.temp_dir ().absoluteFilePath (".lock")}.remove(); // Allow jt9 to terminate
  bool b=proc_jt9.waitForFinished(1000);
  if(!b) proc_jt9.close();
  quitFile.remove();

  Q_EMIT finished ();

  QMainWindow::closeEvent (e);
}

void MainWindow::on_labDialFreq_clicked()                       //dialFrequency
{
    ui->bandComboBox->setFocus();
}

void MainWindow::on_stopButton_clicked()                       //stopButton
{
  monitor (false);
  m_loopall=false;
  if(m_bRefSpec) {
    MessageBox::information_message (this, tr ("Reference spectrum saved"));
    m_bRefSpec=false;
  }
}

void MainWindow::on_actionRelease_Notes_triggered ()
{
  QDesktopServices::openUrl (QUrl {"http://physics.princeton.edu/pulsar/k1jt/Release_Notes.txt"});
}

void MainWindow::on_actionFT8_DXpedition_Mode_User_Guide_triggered()
{
  QDesktopServices::openUrl (QUrl {"http://physics.princeton.edu/pulsar/k1jt/FT8_DXpedition_Mode.pdf"});
}
void MainWindow::on_actionOnline_User_Guide_triggered()      //Display manual
{
#if defined (CMAKE_BUILD)
  m_manual.display_html_url (QUrl {PROJECT_MANUAL_DIRECTORY_URL}, PROJECT_MANUAL);
#endif
}

//Display local copy of manual
void MainWindow::on_actionLocal_User_Guide_triggered()
{
#if defined (CMAKE_BUILD)
  m_manual.display_html_file (m_config.doc_dir (), PROJECT_MANUAL);
#endif
}

void MainWindow::on_actionWide_Waterfall_triggered()      //Display Waterfalls
{
  m_wideGraph->show();
}

void MainWindow::on_actionEcho_Graph_triggered()
{
  m_echoGraph->show();
}

void MainWindow::on_actionFast_Graph_triggered()
{
  m_fastGraph->show();
}

void MainWindow::on_actionSolve_FreqCal_triggered()
{
  QString dpath{QDir::toNativeSeparators(m_config.writeable_data_dir().absolutePath()+"/")};
  char data_dir[512];
  int len=dpath.length();
  int iz,irc;
  double a,b,rms,sigmaa,sigmab;
  strncpy(data_dir,dpath.toLatin1(),len);
  calibrate_(data_dir,&iz,&a,&b,&rms,&sigmaa,&sigmab,&irc,len);
  QString t2;
  if(irc==-1) t2="Cannot open " + dpath + "fmt.all";
  if(irc==-2) t2="Cannot open " + dpath + "fcal2.out";
  if(irc==-3) t2="Insufficient data in fmt.all";
  if(irc==-4) t2 = tr ("Invalid data in fmt.all at line %1").arg (iz);
  if(irc>0 or rms>1.0) t2="Check fmt.all for possible bad data.";
  if (irc < 0 || irc > 0 || rms > 1.) {
    MessageBox::warning_message (this, "Calibration Error", t2);
  }
  else if (MessageBox::Apply == MessageBox::query_message (this
                                                           , tr ("Good Calibration Solution")
                                                           , tr ("<pre>"
                                                                 "%1%L2 ±%L3 ppm\n"
                                                                 "%4%L5 ±%L6 Hz\n\n"
                                                                 "%7%L8\n"
                                                                 "%9%L10 Hz"
                                                                 "</pre>")
                                                           .arg ("Slope: ", 12).arg (b, 0, 'f', 3).arg (sigmab, 0, 'f', 3)
                                                           .arg ("Intercept: ", 12).arg (a, 0, 'f', 2).arg (sigmaa, 0, 'f', 2)
                                                           .arg ("N: ", 12).arg (iz)
                                                           .arg ("StdDev: ", 12).arg (rms, 0, 'f', 2)
                                                           , QString {}
                                                           , MessageBox::Cancel | MessageBox::Apply)) {
    m_config.set_calibration (Configuration::CalibrationParams {a, b});
    if (MessageBox::Yes == MessageBox::query_message (this
                                                      , tr ("Delete Calibration Measurements")
                                                      , tr ("The \"fmt.all\" file will be renamed as \"fmt.bak\""))) {
      // rename fmt.all as we have consumed the resulting calibration
      // solution
      auto const& backup_file_name = m_config.writeable_data_dir ().absoluteFilePath ("fmt.bak");
      QFile::remove (backup_file_name);
      QFile::rename (m_config.writeable_data_dir ().absoluteFilePath ("fmt.all"), backup_file_name);
    }
  }
}

void MainWindow::on_actionCopyright_Notice_triggered()
{
  auto const& message = tr("If you make fair use of any part of this program under terms of the GNU "
                           "General Public License, you must display the following copyright "
                           "notice prominently in your derivative work:\n\n"
                           "\"The algorithms, source code, look-and-feel of WSJT-X and related "
                           "programs, and protocol specifications for the modes FSK441, FT8, JT4, "
                           "JT6M, JT9, JT65, JTMS, QRA64, ISCAT, MSK144 are Copyright (C) "
                           "2001-2018 by one or more of the following authors: Joseph Taylor, "
                           "K1JT; Bill Somerville, G4WJS; Steven Franke, K9AN; Nico Palermo, "
                           "IV3NWV; Greg Beam, KI7MT; Michael Black, W9MDB; Edson Pereira, PY2SDR; "
                           "Philip Karn, KA9Q; and other members of the WSJT Development Group.\n\n"
                           "Further, the source code of FT8Call contains material Copyright (C) "
                           "2018 by Jordan Sherer, KN4CRD.\"");
  MessageBox::warning_message(this, message);
}

// This allows the window to shrink by removing certain things
// and reducing space used by controls
void MainWindow::hideMenus(bool checked)
{
  int spacing = checked ? 1 : 6;
  if (checked) {
      statusBar ()->removeWidget (&auto_tx_label);
      minimumSize().setHeight(450);
      minimumSize().setWidth(700);
      restoreGeometry(m_geometryNoControls);
      updateGeometry();
  } else {
      m_geometryNoControls = saveGeometry();
      statusBar ()->addWidget(&auto_tx_label);
      minimumSize().setHeight(520);
      minimumSize().setWidth(770);
  }
  ui->menuBar->setVisible(!checked);
  if(m_mode!="FreqCal" and m_mode!="WSPR") {
    ui->label_6->setVisible(!checked);
    ui->label_7->setVisible(!checked);
    ui->decodedTextLabel2->setVisible(!checked);
//    ui->line_2->setVisible(!checked);
  }
//  ui->line->setVisible(!checked);
  ui->decodedTextLabel->setVisible(!checked);
  ui->gridLayout_5->layout()->setSpacing(spacing);
  ui->horizontalLayout->layout()->setSpacing(spacing);
  ui->horizontalLayout_2->layout()->setSpacing(spacing);
  ui->horizontalLayout_3->layout()->setSpacing(spacing);
  ui->horizontalLayout_5->layout()->setSpacing(spacing);
  ui->horizontalLayout_6->layout()->setSpacing(spacing);
  ui->horizontalLayout_7->layout()->setSpacing(spacing);
  ui->horizontalLayout_8->layout()->setSpacing(spacing);
  ui->horizontalLayout_9->layout()->setSpacing(spacing);
  ui->horizontalLayout_10->layout()->setSpacing(spacing);
  ui->horizontalLayout_11->layout()->setSpacing(spacing);
  ui->horizontalLayout_12->layout()->setSpacing(spacing);
  ui->horizontalLayout_13->layout()->setSpacing(spacing);
  ui->horizontalLayout_14->layout()->setSpacing(spacing);
  ui->verticalLayout->layout()->setSpacing(spacing);
  ui->verticalLayout_2->layout()->setSpacing(spacing);
  ui->verticalLayout_3->layout()->setSpacing(spacing);
  ui->verticalLayout_4->layout()->setSpacing(spacing);
  ui->verticalLayout_5->layout()->setSpacing(spacing);
  ui->verticalLayout_7->layout()->setSpacing(spacing);
  ui->verticalLayout_8->layout()->setSpacing(spacing);
  ui->tab->layout()->setSpacing(spacing);
}

void MainWindow::on_actionAstronomical_data_toggled (bool checked)
{
  if (checked)
    {
      m_astroWidget.reset (new Astro {m_settings, &m_config});

      // hook up termination signal
      connect (this, &MainWindow::finished, m_astroWidget.data (), &Astro::close);
      connect (m_astroWidget.data (), &Astro::tracking_update, [this] {
          m_astroCorrection = {};
          setRig ();
          setXIT (ui->TxFreqSpinBox->value ());
          displayDialFrequency ();
        });
      m_astroWidget->showNormal();
      m_astroWidget->raise ();
      m_astroWidget->activateWindow ();
      m_astroWidget->nominal_frequency (m_freqNominal, m_freqTxNominal);
    }
  else
    {
      m_astroWidget.reset ();
    }
}

void MainWindow::on_actionFox_Log_triggered()
{
  on_actionMessage_averaging_triggered();
  m_msgAvgWidget->foxLogSetup();
}

void MainWindow::on_actionMessage_averaging_triggered()
{
  if (!m_msgAvgWidget)
    {
      m_msgAvgWidget.reset (new MessageAveraging {m_settings, m_config.decoded_text_font ()});

      // Connect signals from Message Averaging window
      connect (this, &MainWindow::finished, m_msgAvgWidget.data (), &MessageAveraging::close);
    }
  m_msgAvgWidget->showNormal();
  m_msgAvgWidget->raise ();
  m_msgAvgWidget->activateWindow ();
}

void MainWindow::on_actionOpen_triggered()                     //Open File
{
  monitor (false);

  QString fname;
  fname=QFileDialog::getOpenFileName(this, "Open File", m_path,
                                     "WSJT Files (*.wav)");
  if(!fname.isEmpty ()) {
    m_path=fname;
    int i1=fname.lastIndexOf("/");
    QString baseName=fname.mid(i1+1);
    tx_status_label.setStyleSheet("QLabel{background-color: #99ffff}");
    tx_status_label.setText(" " + baseName + " ");
    on_stopButton_clicked();
    m_diskData=true;
    read_wav_file (fname);
  }
}

void MainWindow::read_wav_file (QString const& fname)
{
  // call diskDat() when done
  int i0=fname.lastIndexOf("_");
  int i1=fname.indexOf(".wav");
  m_nutc0=m_UTCdisk;
  m_UTCdisk=fname.mid(i0+1,i1-i0-1).toInt();
  m_wav_future_watcher.setFuture (QtConcurrent::run ([this, fname] {
        auto basename = fname.mid (fname.lastIndexOf ('/') + 1);
        auto pos = fname.indexOf (".wav", 0, Qt::CaseInsensitive);
        // global variables and threads do not mix well, this needs changing
        dec_data.params.nutc = 0;
        if (pos > 0)
          {
            if (pos == fname.indexOf ('_', -11) + 7)
              {
                dec_data.params.nutc = fname.mid (pos - 6, 6).toInt ();
              }
            else
              {
                dec_data.params.nutc = 100 * fname.mid (pos - 4, 4).toInt ();
              }
          }
        BWFFile file {QAudioFormat {}, fname};
        bool ok=file.open (BWFFile::ReadOnly);
        if(ok) {
          auto bytes_per_frame = file.format ().bytesPerFrame ();
          qint64 max_bytes = std::min (std::size_t (m_TRperiod * RX_SAMPLE_RATE),
              sizeof (dec_data.d2) / sizeof (dec_data.d2[0]))* bytes_per_frame;
          auto n = file.read (reinterpret_cast<char *> (dec_data.d2),
                            std::min (max_bytes, file.size ()));
          int frames_read = n / bytes_per_frame;
        // zero unfilled remaining sample space
          std::memset(&dec_data.d2[frames_read],0,max_bytes - n);
          if (11025 == file.format ().sampleRate ()) {
            short sample_size = file.format ().sampleSize ();
            wav12_ (dec_data.d2, dec_data.d2, &frames_read, &sample_size);
          }
          dec_data.params.kin = frames_read;
          dec_data.params.newdat = 1;
        } else {
          dec_data.params.kin = 0;
          dec_data.params.newdat = 0;
        }

        if(basename.mid(0,10)=="000000_000" && m_mode == "FT8") {
          dec_data.params.nutc=15*basename.mid(10,3).toInt();
        }

      }));
}

void MainWindow::on_actionOpen_next_in_directory_triggered()   //Open Next
{
  monitor (false);

  int i,len;
  QFileInfo fi(m_path);
  QStringList list;
  list= fi.dir().entryList().filter(".wav",Qt::CaseInsensitive);
  for (i = 0; i < list.size()-1; ++i) {
    len=list.at(i).length();
    if(list.at(i)==m_path.right(len)) {
      int n=m_path.length();
      QString fname=m_path.replace(n-len,len,list.at(i+1));
      m_path=fname;
      int i1=fname.lastIndexOf("/");
      QString baseName=fname.mid(i1+1);
      tx_status_label.setStyleSheet("QLabel{background-color: #99ffff}");
      tx_status_label.setText(" " + baseName + " ");
      m_diskData=true;
      read_wav_file (fname);
      if(m_loopall and (i==list.size()-2)) {
        m_loopall=false;
        m_bNoMoreFiles=true;
      }
      return;
    }
  }
}
//Open all remaining files
void MainWindow::on_actionDecode_remaining_files_in_directory_triggered()
{
  m_loopall=true;
  on_actionOpen_next_in_directory_triggered();
}

void MainWindow::diskDat()                                   //diskDat()
{
  if(dec_data.params.kin>0) {
    int k;
    int kstep=m_FFTSize;
    m_diskData=true;
    float db=m_config.degrade();
    float bw=m_config.RxBandwidth();
    if(db > 0.0) degrade_snr_(dec_data.d2,&dec_data.params.kin,&db,&bw);
    for(int n=1; n<=m_hsymStop; n++) {                      // Do the waterfall spectra
      k=(n+1)*kstep;
      if(k > dec_data.params.kin) break;
      dec_data.params.npts8=k/8;
      dataSink(k);
      qApp->processEvents();                                //Update the waterfall
    }
  } else {
    MessageBox::information_message(this, tr("No data read from disk. Wrong file format?"));
  }
}

//Delete ../save/*.wav
void MainWindow::on_actionDelete_all_wav_files_in_SaveDir_triggered()
{
  auto button = MessageBox::query_message (this, tr ("Confirm Delete"),
                                             tr ("Are you sure you want to delete all *.wav and *.c2 files in \"%1\"?")
                                             .arg (QDir::toNativeSeparators (m_config.save_directory ().absolutePath ())));
  if (MessageBox::Yes == button) {
    Q_FOREACH (auto const& file
               , m_config.save_directory ().entryList ({"*.wav", "*.c2"}, QDir::Files | QDir::Writable)) {
      m_config.save_directory ().remove (file);
    }
  }
}

void MainWindow::on_actionNone_triggered()                    //Save None
{
  m_saveDecoded=false;
  m_saveAll=false;
  ui->actionNone->setChecked(true);
}

void MainWindow::on_actionSave_decoded_triggered()
{
  m_saveDecoded=true;
  m_saveAll=false;
  ui->actionSave_decoded->setChecked(true);
}

void MainWindow::on_actionSave_all_triggered()                //Save All
{
  m_saveDecoded=false;
  m_saveAll=true;
  ui->actionSave_all->setChecked(true);
}

void MainWindow::on_actionKeyboard_shortcuts_triggered()
{
  if (!m_shortcuts)
    {
      QFont font;
      font.setPointSize (10);
      m_shortcuts.reset (new HelpTextWindow {tr ("Keyboard Shortcuts")
            , ":/shortcuts.txt", font});
    }
  m_shortcuts->showNormal ();
  m_shortcuts->raise ();
}

void MainWindow::on_actionSpecial_mouse_commands_triggered()
{
  if (!m_mouseCmnds)
    {
      QFont font;
      font.setPointSize (10);
      m_mouseCmnds.reset (new HelpTextWindow {tr ("Special Mouse Commands")
            , ":/mouse_commands.txt", font});
    }
  m_mouseCmnds->showNormal ();
  m_mouseCmnds->raise ();
}

void MainWindow::on_DecodeButton_clicked (bool /* checked */) //Decode request
{
  if(m_mode=="MSK144") {
    ui->DecodeButton->setChecked(false);
  } else {
    if(!m_mode.startsWith ("WSPR") && !m_decoderBusy) {
      dec_data.params.newdat=0;
      dec_data.params.nagain=1;
      m_blankLine=false; // don't insert the separator again
      decode();
    }
  }
}

void MainWindow::freezeDecode(int n)                          //freezeDecode()
{
  if((n%100)==2) on_DecodeButton_clicked (true);
}

void MainWindow::on_ClrAvgButton_clicked()
{
  m_nclearave=1;
  if(m_msgAvgWidget != NULL) {
    if(m_msgAvgWidget->isVisible()) m_msgAvgWidget->displayAvg("");
  }
}

void MainWindow::msgAvgDecode2()
{
  on_DecodeButton_clicked (true);
}

void MainWindow::decode()                                       //decode()
{
  QDateTime now = QDateTime::currentDateTime();
  if( m_dateTimeLastTX.isValid () ) {
    qint64 isecs_since_tx = m_dateTimeLastTX.secsTo(now);
    dec_data.params.lapcqonly= (isecs_since_tx > 600);
//    QTextStream(stdout) << "last tx " << isecs_since_tx << endl;
  } else {
    m_dateTimeLastTX = now.addSecs(-900);
    dec_data.params.lapcqonly=true;
  }
  if( m_diskData ) {
    dec_data.params.lapcqonly=false;
  }

  m_msec0=QDateTime::currentMSecsSinceEpoch();
  if(!m_dataAvailable or m_TRperiod==0) return;
  ui->DecodeButton->setChecked (true);
  if(!dec_data.params.nagain && m_diskData && !m_bFastMode && m_mode!="FT8") {
    dec_data.params.nutc=dec_data.params.nutc/100;
  }
  if(dec_data.params.nagain==0 && dec_data.params.newdat==1 && (!m_diskData)) {
    qint64 ms = QDateTime::currentMSecsSinceEpoch() % 86400000;
    int imin=ms/60000;
    int ihr=imin/60;
    imin=imin % 60;
    if(m_TRperiod>=60) imin=imin - (imin % (m_TRperiod/60));
    dec_data.params.nutc=100*ihr + imin;
    if(m_mode=="ISCAT" or m_mode=="MSK144" or m_bFast9 or m_mode=="FT8") {
      QDateTime t=QDateTime::currentDateTimeUtc().addSecs(2-m_TRperiod);
      ihr=t.toString("hh").toInt();
      imin=t.toString("mm").toInt();
      int isec=t.toString("ss").toInt();
      isec=isec - isec%m_TRperiod;
      dec_data.params.nutc=10000*ihr + 100*imin + isec;
    }
  }

  if(m_nPick==1 and !m_diskData) {
    QDateTime t=QDateTime::currentDateTimeUtc();
    int ihr=t.toString("hh").toInt();
    int imin=t.toString("mm").toInt();
    int isec=t.toString("ss").toInt();
    isec=isec - isec%m_TRperiod;
    dec_data.params.nutc=10000*ihr + 100*imin + isec;
  }
  if(m_nPick==2) dec_data.params.nutc=m_nutc0;
  dec_data.params.nQSOProgress = m_QSOProgress;
  dec_data.params.nfqso=m_wideGraph->rxFreq();
  dec_data.params.nftx = ui->TxFreqSpinBox->value ();
  qint32 depth {m_ndepth};
  if (!ui->actionInclude_averaging->isVisible ()) depth &= ~16;
  if (!ui->actionInclude_correlation->isVisible ()) depth &= ~32;
  if (!ui->actionEnable_AP_DXcall->isVisible ()) depth &= ~64;
  dec_data.params.ndepth=depth;
  dec_data.params.n2pass=1;
  if(m_config.twoPass()) dec_data.params.n2pass=2;
  dec_data.params.nranera=m_config.ntrials();
  dec_data.params.naggressive=m_config.aggressive();
  dec_data.params.nrobust=0;
  dec_data.params.ndiskdat=0;
  if(m_diskData) dec_data.params.ndiskdat=1;
  dec_data.params.nfa=m_wideGraph->nStartFreq();
  dec_data.params.nfSplit=m_wideGraph->Fmin();
  dec_data.params.nfb=m_wideGraph->Fmax();
  if(m_mode=="FT8" and m_config.bHound() and !ui->cbRxAll->isChecked()) dec_data.params.nfb=1000;
  if(m_mode=="FT8" and m_config.bFox()) dec_data.params.nfqso=200;
  dec_data.params.ntol=ui->sbFtol->value ();
  if(m_mode=="JT9+JT65" or !m_config.enable_VHF_features()) {
    dec_data.params.ntol=20;
    dec_data.params.naggressive=0;
  }
  if(dec_data.params.nutc < m_nutc0) m_RxLog = 1;       //Date and Time to ALL.TXT
  if(dec_data.params.newdat==1 and !m_diskData) m_nutc0=dec_data.params.nutc;
  dec_data.params.ntxmode=9;
  if(m_modeTx=="JT65") dec_data.params.ntxmode=65;
  dec_data.params.nmode=9;
  if(m_mode=="JT65") dec_data.params.nmode=65;
  if(m_mode=="JT65") dec_data.params.ljt65apon = ui->actionEnable_AP_JT65->isVisible () && ui->actionEnable_AP_JT65->isChecked ();
  if(m_mode=="QRA64") dec_data.params.nmode=164;
  if(m_mode=="QRA64") dec_data.params.ntxmode=164;
  if(m_mode=="JT9+JT65") dec_data.params.nmode=9+65;  // = 74
  if(m_mode=="JT4") {
    dec_data.params.nmode=4;
    dec_data.params.ntxmode=4;
  }
  if(m_mode=="FT8") dec_data.params.nmode=8;
  if(m_mode=="FT8") dec_data.params.lft8apon = ui->actionEnable_AP_FT8->isVisible () && ui->actionEnable_AP_FT8->isChecked ();
  if(m_mode=="FT8") dec_data.params.napwid=50;
  dec_data.params.ntrperiod=m_TRperiod;
  dec_data.params.nsubmode=m_nSubMode;
  if(m_mode=="QRA64") dec_data.params.nsubmode=100 + m_nSubMode;
  dec_data.params.minw=0;
  dec_data.params.nclearave=m_nclearave;
  if(m_nclearave!=0) {
    QFile f(m_config.temp_dir ().absoluteFilePath ("avemsg.txt"));
    f.remove();
  }
  dec_data.params.dttol=m_DTtol;
  dec_data.params.emedelay=0.0;
  if(m_config.decode_at_52s()) dec_data.params.emedelay=2.5;
  dec_data.params.minSync=ui->syncSpinBox->isVisible () ? m_minSync : 0;
  dec_data.params.nexp_decode=0;
  if(m_config.single_decode()) dec_data.params.nexp_decode += 32;
  if(m_config.enable_VHF_features()) dec_data.params.nexp_decode += 64;
  if(ui->cbVHFcontest->isChecked()) dec_data.params.nexp_decode += 128;

  strncpy(dec_data.params.datetime, m_dateTime.toLatin1(), 20);
  strncpy(dec_data.params.mycall, (m_config.my_callsign()+"            ").toLatin1(),12);
  strncpy(dec_data.params.mygrid, (m_config.my_grid()+"      ").toLatin1(),6);
  QString hisCall {ui->dxCallEntry->text ()};
  QString hisGrid {ui->dxGridEntry->text ()};
  strncpy(dec_data.params.hiscall,(hisCall + "            ").toLatin1 ().constData (), 12);
  strncpy(dec_data.params.hisgrid,(hisGrid + "      ").toLatin1 ().constData (), 6);

  //newdat=1  ==> this is new data, must do the big FFT
  //nagain=1  ==> decode only at fQSO +/- Tol

  char *to = (char*)mem_jt9->data();
  char *from = (char*) dec_data.ss;
  int size=sizeof(struct dec_data);
  if(dec_data.params.newdat==0) {
    int noffset {offsetof (struct dec_data, params.nutc)};
    to += noffset;
    from += noffset;
    size -= noffset;
  }
  if(m_mode=="ISCAT" or m_mode=="MSK144" or m_bFast9) {
    float t0=m_t0;
    float t1=m_t1;
    qApp->processEvents();                                //Update the waterfall
    if(m_nPick > 0) {
      t0=m_t0Pick;
      t1=m_t1Pick;
    }
    static short int d2b[360000];
    narg[0]=dec_data.params.nutc;
    if(m_kdone>12000*m_TRperiod) {
      m_kdone=12000*m_TRperiod;
    }
    narg[1]=m_kdone;
    narg[2]=m_nSubMode;
    narg[3]=dec_data.params.newdat;
    narg[4]=dec_data.params.minSync;
    narg[5]=m_nPick;
    narg[6]=1000.0*t0;
    narg[7]=1000.0*t1;
    narg[8]=2;                                //Max decode lines per decode attempt
    if(dec_data.params.minSync<0) narg[8]=50;
    if(m_mode=="ISCAT") narg[9]=101;          //ISCAT
    if(m_mode=="JT9") narg[9]=102;            //Fast JT9
    if(m_mode=="MSK144") narg[9]=104;         //MSK144
    narg[10]=ui->RxFreqSpinBox->value();
    narg[11]=ui->sbFtol->value ();
    narg[12]=0;
    narg[13]=-1;
    narg[14]=m_config.aggressive();
    memcpy(d2b,dec_data.d2,2*360000);
    watcher3.setFuture (QtConcurrent::run (std::bind (fast_decode_,&d2b[0],
        &narg[0],&m_TRperiod,&m_msg[0][0],
        dec_data.params.mycall,dec_data.params.hiscall,8000,12,12)));
  } else {
    memcpy(to, from, qMin(mem_jt9->size(), size));
    QFile {m_config.temp_dir ().absoluteFilePath (".lock")}.remove (); // Allow jt9 to start
    decodeBusy(true);
  }
}

void::MainWindow::fast_decode_done()
{
  float t,tmax=-99.0;
  dec_data.params.nagain=false;
  dec_data.params.ndiskdat=false;
//  if(m_msg[0][0]==0) m_bDecoded=false;
  for(int i=0; m_msg[i][0] && i<100; i++) {
    QString message=QString::fromLatin1(m_msg[i]);
    m_msg[i][0]=0;
    if(message.length()>80) message=message.left (80);
    if(narg[13]/8==narg[12]) message=message.trimmed().replace("<...>",m_calls);

//Left (Band activity) window
    DecodedText decodedtext {message.replace (QChar::LineFeed, ""), "FT8" == m_mode &&
          ui->cbVHFcontest->isChecked(), m_config.my_grid ()};
    if(!m_bFastDone) {
      ui->decodedTextBrowser->displayDecodedText (decodedtext,m_baseCall,m_config.DXCC(),
         m_logBook,m_config.color_CQ(),m_config.color_MyCall(),m_config.color_DXCC(),
         m_config.color_NewCall(),m_config.ppfx());
    }

    t=message.mid(10,5).toFloat();
    if(t>tmax) {
      tmax=t;
      m_bDecoded=true;
    }
    postDecode (true, decodedtext.string ());
    writeAllTxt(message);

    if(m_mode=="JT9" or m_mode=="MSK144") {
      // find and extract any report for myCall
      bool stdMsg = decodedtext.report(m_baseCall,
                    Radio::base_callsign(ui->dxCallEntry->text()), m_rptRcvd);

      // extract details and send to PSKreporter
      //if (stdMsg) pskPost (decodedtext);
    }
    if (tmax >= 0.0) auto_sequence (decodedtext, ui->sbFtol->value (), ui->sbFtol->value ());
  }
  m_startAnother=m_loopall;
  m_nPick=0;
  ui->DecodeButton->setChecked (false);
  m_bFastDone=false;
}

void MainWindow::writeAllTxt(QString message)
{
  // Write decoded text to file "ALL.TXT".
  QFile f {m_config.writeable_data_dir ().absoluteFilePath ("ALL.TXT")};
      if (f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
        QTextStream out(&f);
        if(m_RxLog==1) {
          out << QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd hh:mm")
              << "  " << qSetRealNumberPrecision (12) << (m_freqNominal / 1.e6) << " MHz  "
              << m_mode << endl;
          m_RxLog=0;
        }
        auto dt = DecodedText(message);
        out << dt.message() << endl;
        f.close();
      } else {
        MessageBox::warning_message (this, tr ("File Open Error")
                                     , tr ("Cannot open \"%1\" for append: %2")
                                     .arg (f.fileName ()).arg (f.errorString ()));
      }
}

void MainWindow::decodeDone ()
{
  dec_data.params.nagain=0;
  dec_data.params.ndiskdat=0;
  m_nclearave=0;
  QFile {m_config.temp_dir ().absoluteFilePath (".lock")}.open(QIODevice::ReadWrite);
  ui->DecodeButton->setChecked (false);
  decodeBusy(false);
  m_RxLog=0;
  m_blankLine=true;
  if(m_config.bFox()) houndCallers();
}

void MainWindow::readFromStdout()                             //readFromStdout
{
  while(proc_jt9.canReadLine()) {
    QByteArray t=proc_jt9.readLine();
    qDebug() << "JT9: " << QString(t);
    if(m_mode=="FT8" and !m_config.bHound() and t.contains(";")) {
      if(t.contains("<...>")) continue;
      if(!m_bWarnedHound) {
        QString errorMsg;
        MessageBox::critical_message (this,
           tr("Should you be in \"FT8 DXpedition Hound\" mode?"), errorMsg);
        m_bWarnedHound=true;
      }
    }
//    qint64 ms=QDateTime::currentMSecsSinceEpoch() - m_msec0;
    bool bAvgMsg=false;
    int navg=0;
    if(t.indexOf("<DecodeFinished>") >= 0) {
      if(m_mode=="QRA64") m_wideGraph->drawRed(0,0);
      m_bDecoded = t.mid(20).trimmed().toInt() > 0;
      int mswait=3*1000*m_TRperiod/4;
      if(!m_diskData) killFileTimer.start(mswait); //Kill in 3/4 period
      decodeDone ();
      m_startAnother=m_loopall;
      if(m_bNoMoreFiles) {
        MessageBox::information_message(this, tr("No more files to open."));
        m_bNoMoreFiles=false;
      }
      return;
    } else {
      if(m_mode=="JT4" or m_mode=="JT65" or m_mode=="QRA64" or m_mode=="FT8") {
        int n=t.indexOf("f");
        if(n<0) n=t.indexOf("d");
        if(n>0) {
          QString tt=t.mid(n+1,1);
          navg=tt.toInt();
          if(navg==0) {
            char c = tt.data()->toLatin1();
            if(int(c)>=65 and int(c)<=90) navg=int(c)-54;
          }
          if(navg>1 or t.indexOf("f*")>0) bAvgMsg=true;
        }
      }

      QFile f {m_config.writeable_data_dir ().absoluteFilePath ("ALL.TXT")};
      if (f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
        QTextStream out(&f);
        if(m_RxLog==1) {
          out << QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd hh:mm")
              << "  " << qSetRealNumberPrecision (12) << (m_freqNominal / 1.e6) << " MHz  "
              << m_mode << endl;
          m_RxLog=0;
        }
        int n=t.length();
        auto logText = t.mid(0, n-2);
        auto dt = DecodedText(logText, false, m_config.my_grid());
        out << logText << "  " << dt.message() << endl;
        f.close();
      } else {
        MessageBox::warning_message (this, tr ("File Open Error")
                                     , tr ("Cannot open \"%1\" for append: %2")
                                     .arg (f.fileName ()).arg (f.errorString ()));
      }
      if (m_config.insert_blank () && m_blankLine && !m_config.bFox()) {
        QString band;
        if((QDateTime::currentMSecsSinceEpoch() / 1000 - m_secBandChanged) > 4*m_TRperiod/4) {
          band = ' ' + m_config.bands ()->find (m_freqNominal);
        }
        ui->decodedTextBrowser->insertLineSpacer (band.rightJustified  (40, '-'));
        m_blankLine = false;
      }

      DecodedText decodedtext {QString::fromUtf8 (t.constData ()).remove (QRegularExpression {"\r|\n"}), "FT8" == m_mode &&
            ui->cbVHFcontest->isChecked(), m_config.my_grid ()};

      if(m_mode=="FT8" and m_config.bFox() and
         (decodedtext.string().contains("R+") or decodedtext.string().contains("R-"))) {
        auto for_us  = decodedtext.string().contains(" " + m_config.my_callsign() + " ") or
            decodedtext.string().contains(" "+m_baseCall) or
            decodedtext.string().contains(m_baseCall+" ");
        if(decodedtext.string().contains(" DE ")) for_us=true;   //Hound with compound callsign
        if(for_us) {
          QString houndCall,houndGrid;
          decodedtext.deCallAndGrid(/*out*/houndCall,houndGrid);
          foxRxSequencer(decodedtext.string(),houndCall,houndGrid);
        }
      }

      // only display frames that are FT8Call frames (should decrease false decodes by at least 12%)
      int bits = decodedtext.bits();

      bool bValidFrame = (
        bits == Varicode::FT8Call                                            ||
        ((bits & Varicode::FT8CallFirst)    == Varicode::FT8CallFirst)       ||
        ((bits & Varicode::FT8CallLast)     == Varicode::FT8CallLast)        ||
        ((bits & Varicode::FT8CallReserved) == 0 /*Varicode::FT8CallReserved*/) // This is unused...so is invalid at this time...
      );

      qDebug() << "valid" << bValidFrame << "decoded text" << decodedtext.message();

      //Left (Band activity) window
      if(bValidFrame) {
          // Parse General Activity
#if 1
          bool shouldParseGeneralActivity = true;
          if(shouldParseGeneralActivity && !decodedtext.messageWords().isEmpty()){
            int offset = decodedtext.frequencyOffset();

            if(!m_bandActivity.contains(offset)){
                QList<int> offsets = {
                    offset - 1, offset - 2, offset - 3, offset - 4, offset - 5, offset - 6, offset - 7, offset - 8, offset - 9, offset - 10,
                    offset + 1, offset + 2, offset + 3, offset + 4, offset + 5, offset + 6, offset + 7, offset + 8, offset + 9, offset + 10
                };
                foreach(int prevOffset, offsets){
                    if(!m_bandActivity.contains(prevOffset)){ continue; }
                    m_bandActivity[offset] = m_bandActivity[prevOffset];
                    m_bandActivity.remove(prevOffset);
                    break;
                }
            }

            ActivityDetail d = {};
            d.isLowConfidence = decodedtext.isLowConfidence();
            d.isFree = !decodedtext.isStandardMessage();
            d.isCompound = decodedtext.isCompound();
            d.isDirected = decodedtext.isDirectedMessage();
            d.bits = decodedtext.bits();
            d.freq = offset;
            d.text = decodedtext.message();
            d.utcTimestamp = QDateTime::currentDateTimeUtc();
            d.snr = decodedtext.snr();
            d.isBuffered = false;

            // if we have any "first" frame, and a buffer is already established, clear it...
            if(((d.bits & Varicode::FT8CallFirst) == Varicode::FT8CallFirst) && m_messageBuffer.contains(d.freq/10*10)){
                qDebug() << "first message encountered, clearing existing buffer" << (d.freq/10*10);
                m_messageBuffer.remove(d.freq/10*10);
            }

            // if we have a data frame, and a message buffer has been established, buffer it...
            if(m_messageBuffer.contains(d.freq/10*10) && !decodedtext.isCompound() && !decodedtext.isDirectedMessage()){
                qDebug() << "buffering data" << (d.freq/10*10) << d.text;
                d.isBuffered = true;
                m_messageBuffer[d.freq/10*10].msgs.append(d);
            }

            m_rxActivityQueue.append(d);
            m_bandActivity[offset].append(d);
            while(m_bandActivity[offset].count() > 10){
                m_bandActivity[offset].removeFirst();
            }
          }

#endif


          // Process compound callsign commands (put them in cache)"
#if 1
          qDebug() << "decoded" << decodedtext.frameType() << decodedtext.isCompound() << decodedtext.isDirectedMessage() << decodedtext.isBeacon();
          bool shouldProcessCompound = true;
          if(shouldProcessCompound && decodedtext.isCompound() && !decodedtext.isDirectedMessage()){
            CallDetail cd = {};
            cd.call = decodedtext.compoundCall();
            cd.grid = decodedtext.extra(); // compound calls via beacons may contain grid...
            cd.snr = decodedtext.snr();
            cd.freq = decodedtext.frequencyOffset();
            cd.utcTimestamp = QDateTime::currentDateTimeUtc();
            cd.bits = decodedtext.bits();

            if(decodedtext.isBeacon()){
                logCallActivity(cd, true);
            } else {
                qDebug() << "buffering compound call" << cd.call << cd.bits;
                m_messageBuffer[cd.freq/10*10].compound.append(cd);
            }
          }
#endif

          // Parse commands
          // KN4CRD K1JT ?
#if 1
          bool shouldProcessDirected = true;
          if(shouldProcessDirected && decodedtext.isDirectedMessage()){
              auto parts = decodedtext.directedMessage();

              CommandDetail d = {};
              d.from = parts.at(0);
              d.to = parts.at(1);
              d.cmd = parts.at(2);
              d.freq = decodedtext.frequencyOffset();
              d.snr = decodedtext.snr();
              d.utcTimestamp = QDateTime::currentDateTimeUtc();
              d.bits = decodedtext.bits();
              d.extra = parts.length() > 2 ? parts.mid(3).join(" ") : "";

              // if the command is a buffered command OR we have from or to in a separate message (compound)
              if(Varicode::isCommandBuffered(d.cmd) || d.from == "<....>" || d.to == "<....>"){
                qDebug() << "buffering cmd" << d.cmd << d.from << d.to;
                m_messageBuffer[d.freq/10*10].cmd = d;
                m_messageBuffer[d.freq/10*10].msgs.clear();
              } else {
                m_rxCommandQueue.append(d);
              }

              /*
              // DISABLED FOR NOW...
              CallDetail cd;
              cd.bits = d.bits;
              cd.call = d.from;
              cd.grid = "";
              cd.snr = d.snr;
              cd.freq = d.freq;
              cd.utcTimestamp = d.utcTimestamp;
              logCallActivity(cd);
              */

#if 0
              bool shouldCaptureThirdPartyCallsigns = false;
              // check to see if this is a station we've heard 3rd party
              if(shouldCaptureThirdPartyCallsigns && Radio::base_callsign(d.to) != Radio::base_callsign(m_config.my_callsign())){
                  QString relayCall = QString("%1|%2").arg(Radio::base_callsign(d.from)).arg(Radio::base_callsign(d.to));
                  int snr = -100;
                  if(parts.length() == 4){
                      snr = QString(parts.at(3)).toInt();
                  }
                  CallDetail td;
                  td.through = d.from;
                  td.call = d.to;
                  td.grid = "";
                  td.snr = snr;
                  td.freq = d.freq;
                  td.utcTimestamp = d.utcTimestamp;
                  m_callActivity[relayCall] = td;
              }
#endif
          }
#endif

          // Parse CQs
#if 1
          bool shouldParseCQs = true;
          if(shouldParseCQs && decodedtext.isStandardMessage()){
            QString theircall;
            QString theirgrid;
            decodedtext.deCallAndGrid(theircall, theirgrid);

            QStringList calls = Varicode::parseCallsigns(theircall);
            if(!calls.isEmpty() && !calls.first().isEmpty()){
                theircall = calls.first();

                CallDetail d = {};
                d.bits = decodedtext.bits();
                d.call = theircall;
                d.grid = theirgrid;
                d.snr = decodedtext.snr();
                d.freq = decodedtext.frequencyOffset();
                d.utcTimestamp = QDateTime::currentDateTimeUtc();
                m_callActivity[d.call] = d;
              }
          }
#endif

          // Parse standard message callsigns
          // K1JT KN4CRD EM73
          // KN4CRD K1JT -21
          // K1JT KN4CRD R-12
          // DE KN4CRD
          // KN4CRD
#if 0
          bool shouldParseCallsigns = false;
          if(shouldParseCallsigns){
              QStringList callsigns = Varicode::parseCallsigns(decodedtext.message());
              if(!callsigns.isEmpty()){
                  // one callsign
                  // de [from]
                  // cq [from]

                  // two callsigns
                  // [from]: [to] ...
                  // [to] [from] [grid|signal]

                  QStringList grids = Varicode::parseGrids(decodedtext.message());

                  // one callsigns are handled above... so we only need to handle two callsigns if it's a standard message
                  if(decodedtext.isStandardMessage()){
                      if(callsigns.length() == 2){
                          auto de_callsign = callsigns.last();

                          // TODO: jsherer - put this in a function to record a callsign...
                          CallDetail d;
                          d.call = de_callsign;
                          d.grid = !grids.empty() ? grids.first() : "";
                          d.snr = decodedtext.snr();
                          d.freq = decodedtext.frequencyOffset();
                          d.utcTimestamp = QDateTime::currentDateTimeUtc();
                          m_callActivity[Radio::base_callsign(de_callsign)] = d;
                      }
                  }
              }
          }
#endif
      }

      //Right (Rx Frequency) window
      bool bDisplayRight=bAvgMsg;
      int audioFreq=decodedtext.frequencyOffset();

      if(m_mode=="FT8") {
        auto const& parts = decodedtext.string ().split (' ', QString::SkipEmptyParts);
        if (parts.size () > 6) {
          auto for_us = parts[5].contains (m_baseCall)
            || ("DE" == parts[5] && qAbs (ui->RxFreqSpinBox->value () - audioFreq) <= 10);
          if(m_baseCall==m_config.my_callsign() and m_baseCall!=parts[5]) for_us=false;
          if(m_bCallingCQ && !m_bAutoReply && for_us && ui->cbFirst->isChecked() and
             !m_config.bFox() and !m_config.bHound()) {
            m_bDoubleClicked=true;
            m_bAutoReply = true;
            if(!m_config.bFox()) processMessage (decodedtext);
            ui->cbFirst->setStyleSheet("");
          }
          if(m_config.bFox() and decodedtext.string().contains(" DE ")) for_us=true; //Hound with compound callsign
          if(m_config.bFox() and for_us and (audioFreq<1000)) bDisplayRight=true;
          if(!m_config.bFox() and (for_us or (abs(audioFreq - m_wideGraph->rxFreq()) <= 10))) bDisplayRight=true;
        }
      }

      if(abs(audioFreq - m_wideGraph->rxFreq()) <= 10){
          bDisplayRight=true;
      }

      if (bDisplayRight) {
        // This msg is within 10 hertz of our tuned frequency, or a JT4 or JT65 avg,
        // or contains MyCall
        ui->decodedTextBrowser2->displayDecodedText(decodedtext,m_baseCall,false,
               m_logBook,m_config.color_CQ(),m_config.color_MyCall(),
               m_config.color_DXCC(),m_config.color_NewCall(),m_config.ppfx());

        if(m_mode!="JT4") {
          bool b65=decodedtext.isJT65();
          if(b65 and m_modeTx!="JT65") on_pbTxMode_clicked();
          if(!b65 and m_modeTx=="JT65") on_pbTxMode_clicked();
        }
        m_QSOText = decodedtext.string ().trimmed ();
      }

      if(m_mode=="FT8" and m_config.bHound()) {
        if(decodedtext.string().contains(";")) {
          QStringList w=decodedtext.string().mid(24).split(" ",QString::SkipEmptyParts);
          QString foxCall=w.at(3);
          foxCall=foxCall.remove("<").remove(">");
          if(w.at(0)==m_config.my_callsign() or w.at(0)==Radio::base_callsign(m_config.my_callsign())) {
            //### Check for ui->dxCallEntry->text()==foxCall before logging! ###
            ui->stopTxButton->click ();
            on_logQSOButton_clicked();
          }
          if((w.at(2)==m_config.my_callsign() or w.at(2)==Radio::base_callsign(m_config.my_callsign()))
             and ui->tx3->text().length()>0) {
            m_rptRcvd=w.at(4);
            m_rptSent=decodedtext.string().mid(7,3);
            m_nFoxFreq=decodedtext.string().mid(16,4).toInt();
            hound_reply ();
          }
        } else {
          QStringList w=decodedtext.string().mid(24).split(" ",QString::SkipEmptyParts);
          if(decodedtext.string().contains("/")) w.append(" +00");  //Add a dummy report
          if(w.size()>=3) {
            QString foxCall=w.at(1);
            if((w.at(0)==m_config.my_callsign() or w.at(0)==Radio::base_callsign(m_config.my_callsign())) and
               ui->tx3->text().length()>0) {
              if(w.at(2)=="RR73") {
                ui->stopTxButton->click ();
                on_logQSOButton_clicked();
              } else {
                if(w.at(1)==Radio::base_callsign(ui->dxCallEntry->text()) and
                   (w.at(2).mid(0,1)=="+" or w.at(2).mid(0,1)=="-")) {
                  m_rptRcvd=w.at(2);
                  m_rptSent=decodedtext.string().mid(7,3);
                  m_nFoxFreq=decodedtext.string().mid(16,4).toInt();
                  hound_reply ();
                }
              }
            }
          }
        }
      }

      if(m_mode!="FT8" or !m_config.bHound()) {
        if(m_mode=="FT8" or m_mode=="QRA64" or m_mode=="JT4" or m_mode=="JT65" or
           m_mode=="JT9") auto_sequence (decodedtext, 25, 50);
        postDecode (true, decodedtext.string ());

        // find and extract any report for myCall, but save in m_rptRcvd only if it's from DXcall
        QString rpt;
        bool stdMsg = decodedtext.report(m_baseCall,
            Radio::base_callsign(ui->dxCallEntry->text()), rpt);
        QString deCall;
        QString grid;
        decodedtext.deCallAndGrid(/*out*/deCall,grid);
        {
          QString t=Radio::base_callsign(ui->dxCallEntry->text());
          if((t==deCall or t=="") and rpt!="") m_rptRcvd=rpt;
        }
        // extract details and send to PSKreporter
        int nsec=QDateTime::currentMSecsSinceEpoch()/1000-m_secBandChanged;
        bool okToPost=(nsec>(4*m_TRperiod)/5);

        //if (stdMsg && okToPost) pskPost(decodedtext);

        if((m_mode=="JT4" or m_mode=="JT65" or m_mode=="QRA64") and m_msgAvgWidget!=NULL) {
          if(m_msgAvgWidget->isVisible()) {
            QFile f(m_config.temp_dir ().absoluteFilePath ("avemsg.txt"));
            if(f.open(QIODevice::ReadOnly | QIODevice::Text)) {
              QTextStream s(&f);
              QString t=s.readAll();
              m_msgAvgWidget->displayAvg(t);
            }
          }
        }
      }
    }
  }

  // See MainWindow::postDecode for displaying the latest decodes
}


void MainWindow::logCallActivity(CallDetail d, bool spot){
    if(m_callActivity.contains(d.call)){
        // update (keep grid)
        CallDetail old = m_callActivity[d.call];
        if(d.grid.isEmpty() && !old.grid.isEmpty()){
            d.grid = old.grid;
        }
        m_callActivity[d.call] = d;
    } else {
        // create
        m_callActivity[d.call] = d;
    }

    // enqueue for spotting to psk reporter
    if(spot){
        m_rxCallQueue.append(d);
    }
}

QString MainWindow::lookupCallInCompoundCache(QString const &call){
    QString myBaseCall = Radio::base_callsign(m_config.my_callsign());
    if(call == myBaseCall){
        return m_config.my_callsign();
    }
    return m_compoundCallCache.value(call, call);
}

//
// start_tolerance - only respond to "DE ..." and free text 73
//                   messages within +/- this value
//
// stop_tolerance - kill Tx if running station is seen to reply to
//                  another caller and we are going to transmit within
//                  +/- this value of the reply to another caller
//
void MainWindow::auto_sequence (DecodedText const& message, unsigned start_tolerance, unsigned stop_tolerance)
{
  auto const& message_words = message.messageWords ();
  auto is_73 = message_words.filter (QRegularExpression {"^(73|RR73)$"}).size ();
  bool is_OK=false;
  if(m_mode=="MSK144" and message.string().indexOf(ui->dxCallEntry->text()+" R ")>0) is_OK=true;

  if (message_words.size () > 2 && (message.isStandardMessage () || (is_73 or is_OK))) {
    auto df = message.frequencyOffset ();
    auto within_tolerance =
      (qAbs (ui->RxFreqSpinBox->value () - df) <= int (start_tolerance)
       || qAbs (ui->TxFreqSpinBox->value () - df) <= int (start_tolerance));
    bool acceptable_73 = is_73
      && m_QSOProgress >= ROGER_REPORT
      && ((message.isStandardMessage ()
           && (message_words.contains (m_baseCall)
               || message_words.contains (m_config.my_callsign ())
               || message_words.contains (ui->dxCallEntry->text ())
               || message_words.contains (Radio::base_callsign (ui->dxCallEntry->text ()))
               || message_words.contains ("DE")))
          || !message.isStandardMessage ()); // free text 73/RR73
    if (m_auto
        && (REPLYING == m_QSOProgress
            || (!ui->tx1->isEnabled () && REPORT == m_QSOProgress))
        && qAbs (ui->TxFreqSpinBox->value () - df) <= int (stop_tolerance)
        && message_words.at (1) != "DE"
        && !message_words.at (1).contains (QRegularExpression {"(^(CQ|QRZ))|" + m_baseCall})
        && message_words.at (2).contains (Radio::base_callsign (ui->dxCallEntry->text ()))) {
      // auto stop to avoid accidental QRM
      ui->stopTxButton->click (); // halt any transmission
    }
    else if (m_auto             // transmit allowed
        && ui->cbAutoSeq->isVisible () && ui->cbAutoSeq->isChecked() // auto-sequencing allowed
        && ((!m_bCallingCQ      // not calling CQ/QRZ
             && !m_sentFirst73  // not finished QSO
             && ((message_words.at (1).contains (m_baseCall)
                  // being called and not already in a QSO
                  && message_words.at (2).contains (Radio::base_callsign (ui->dxCallEntry->text ())))
                 // type 2 compound replies
                 || (within_tolerance
                     && (acceptable_73
                         || ("DE" == message_words.at (1) && message_words.at (2).contains (Radio::base_callsign (m_hisCall)))))))
            || (m_bCallingCQ && m_bAutoReply
                // look for type 2 compound call replies on our Tx and Rx offsets
                && ((within_tolerance && "DE" == message_words.at (1))
                    || message_words.at (1).contains (m_baseCall)))))
    {
      if(!m_config.bFox()) processMessage (message);
    }
  }
}

void MainWindow::pskPost (DecodedText const& decodedtext)
{
#if 0
  if (m_diskData || !m_config.spot_to_reporting_networks() || decodedtext.isLowConfidence ()) return;

  QString msgmode=m_mode;
  if(m_mode=="JT9+JT65") {
    msgmode="JT9";
    if (decodedtext.isJT65()) msgmode="JT65";
  }
  QString deCall;
  QString grid;
  decodedtext.deCallAndGrid(/*out*/deCall,grid);
  int audioFrequency = decodedtext.frequencyOffset();
  if(m_mode=="FT8" or m_mode=="MSK144") {
    audioFrequency=decodedtext.string().mid(16,4).toInt();
  }
  int snr = decodedtext.snr();

  pskSetLocal ();
  if(grid.contains (grid_regexp)) {
//    qDebug() << "To PSKreporter:" << deCall << grid << frequency << msgmode << snr;
//    psk_Reporter->addRemoteStation(deCall,grid,QString::number(frequency),msgmode,
//           QString::number(snr),QString::number(QDateTime::currentDateTime().toTime_t()));
      pskLogReport(msgmode, audioFrequency, snr, deCall, grid);
  }
#endif
}

void MainWindow::pskLogReport(QString mode, int offset, int snr, QString callsign, QString grid){
    if(!m_config.spot_to_reporting_networks()) return;

    Frequency frequency = m_freqNominal + offset;

    psk_Reporter->addRemoteStation(
       callsign,
       grid,
       QString::number(frequency),
       mode,
       QString::number(snr),
       QString::number(QDateTime::currentDateTime().toTime_t()));
}

void MainWindow::aprsLogReport(int offset, int snr, QString callsign, QString grid){
    if(!m_config.spot_to_reporting_networks()) return;

    Frequency frequency = m_freqNominal + offset;

    if(grid.length() < 6){
        qDebug() << "APRSISClient Spot Skipped:" << callsign << grid;
        return;
    }

    auto comment = QString("%1MHz %2dB").arg(Radio::frequency_MHz_string(frequency)).arg(Varicode::formatSNR(snr));
    if(callsign.contains("/")){
        comment = QString("%1 %2").arg(callsign).arg(comment);
    }
    m_aprsClient->enqueueSpot(Radio::base_callsign(callsign), grid, comment);
}

void MainWindow::killFile ()
{
  if (m_fnameWE.size () &&
      !(m_saveAll || (m_saveDecoded && m_bDecoded) || m_fnameWE == m_fileToSave)) {
    QFile f1 {m_fnameWE + ".wav"};
    if(f1.exists()) f1.remove();
    if(m_mode.startsWith ("WSPR")) {
      QFile f2 {m_fnameWE + ".c2"};
      if(f2.exists()) f2.remove();
    }
  }
}

void MainWindow::on_EraseButton_clicked ()
{
  qint64 ms=QDateTime::currentMSecsSinceEpoch();
  ui->decodedTextBrowser2->erase ();
  if(m_mode.startsWith ("WSPR") or m_mode=="Echo" or m_mode=="ISCAT") {
    ui->decodedTextBrowser->erase ();
  } else {
    if((ms-m_msErase)<500) {
      ui->decodedTextBrowser->erase ();
    }
  }
  m_msErase=ms;
}

void MainWindow::band_activity_cleared ()
{
#if 0
  m_messageClient->clear_decodes ();
#endif
  QFile f(m_config.temp_dir ().absoluteFilePath ("decoded.txt"));
  if(f.exists()) f.remove();
}

void MainWindow::rx_frequency_activity_cleared ()
{
  m_QSOText.clear();
  set_dateTimeQSO(-1);          // G4WJS: why do we do this?
}

void MainWindow::decodeBusy(bool b)                             //decodeBusy()
{
  if (!b) m_optimizingProgress.reset ();
  m_decoderBusy=b;
  ui->DecodeButton->setEnabled(!b);
  ui->actionOpen->setEnabled(!b);
  ui->actionOpen_next_in_directory->setEnabled(!b);
  ui->actionDecode_remaining_files_in_directory->setEnabled(!b);

  statusUpdate ();
}

//------------------------------------------------------------- //guiUpdate()
void MainWindow::guiUpdate()
{
  static char message[29];
  static char msgsent[29];
  double txDuration;
  QString rt;

  if(m_TRperiod==0) m_TRperiod=60;
  txDuration=0.0;
  if(m_modeTx=="FT8")  txDuration=1.0 + 79*1920/12000.0;      // FT8
  if(m_modeTx=="JT4")  txDuration=1.0 + 207.0*2520/11025.0;   // JT4
  if(m_modeTx=="JT9")  txDuration=1.0 + 85.0*m_nsps/12000.0;  // JT9
  if(m_modeTx=="JT65") txDuration=1.0 + 126*4096/11025.0;     // JT65
  if(m_modeTx=="QRA64")  txDuration=1.0 + 84*6912/12000.0;      // QRA64
  if(m_modeTx=="WSPR") txDuration=2.0 + 162*8192/12000.0;       // WSPR
  if(m_modeTx=="WSPR-LF") txDuration=2.0 + 114*24576/12000.0;   // WSPR-LF
  if(m_modeTx=="ISCAT" or m_mode=="MSK144" or m_bFast9) {
    txDuration=m_TRperiod-0.25; // ISCAT, JT9-fast, MSK144
  }

  double tx1=0.0;
  double tx2=txDuration;
  if(m_mode=="FT8") icw[0]=0;                                   //No CW ID in FT8 mode
  if((icw[0]>0) and (!m_bFast9)) tx2 += icw[0]*2560.0/48000.0;  //Full length including CW ID
  if(tx2>m_TRperiod) tx2=m_TRperiod;

  if(!m_txFirst and !m_mode.startsWith ("WSPR")) {
    tx1 += m_TRperiod;
    tx2 += m_TRperiod;
  }

  qint64 ms = QDateTime::currentMSecsSinceEpoch() % 86400000;
  int nsec=ms/1000;
  double tsec=0.001*ms;
  double t2p=fmod(tsec,2*m_TRperiod);
  m_s6=fmod(tsec,6.0);
  m_nseq = nsec % m_TRperiod;
  m_tRemaining=m_TRperiod - fmod(tsec,double(m_TRperiod));

  if(m_mode=="Echo") {
    txDuration=2.4;
    tx1=0.0;
    tx2=txDuration;
    if(m_auto and m_s6>4.0) m_bEchoTxOK=true;
    if(m_transmitting) m_bEchoTxed=true;
  }

  if(m_mode.startsWith ("WSPR")) {
    if(m_nseq==0 and m_ntr==0) {                   //Decide whether to Tx or Rx
      m_tuneup=false;                              //This is not an ATU tuneup
      if(m_pctx==0) m_WSPR_tx_next = false;        //Don't transmit if m_pctx=0
      bool btx = m_auto && m_WSPR_tx_next;         // To Tx, we need m_auto and
                                                   // scheduled transmit
      if(m_auto and m_txNext) btx=true;            //TxNext button overrides
      if(m_auto and m_pctx==100) btx=true;         //Always transmit

      if(btx) {
        m_ntr=-1;                                  //This says we will have transmitted
        m_txNext=false;
        ui->pbTxNext->setChecked(false);
        m_bTxTime=true;                            //Start a WSPR Tx sequence
      } else {
// This will be a WSPR Rx sequence.
        m_ntr=1;                                   //This says we will have received
        m_bTxTime=false;                           //Start a WSPR Rx sequence
      }
    }

  } else {
// For all modes other than WSPR
    m_bTxTime = (t2p >= tx1) and (t2p < tx2);
    if(m_mode=="Echo") m_bTxTime = m_bTxTime and m_bEchoTxOK;
  }
  if(m_tune) m_bTxTime=true;                 //"Tune" takes precedence

  if(m_transmitting or m_auto or m_tune) {
    m_dateTimeLastTX = QDateTime::currentDateTime ();

// Check for "txboth" (testing purposes only)
    QFile f(m_appDir + "/txboth");
    if(f.exists() and
       fmod(tsec,m_TRperiod)<(1.0 + 85.0*m_nsps/12000.0)) m_bTxTime=true;

// Don't transmit another mode in the 30 m WSPR sub-band
    Frequency onAirFreq = m_freqNominal + ui->TxFreqSpinBox->value();
    if ((onAirFreq > 10139900 and onAirFreq < 10140320) and
        !m_mode.startsWith ("WSPR")) {
      m_bTxTime=false;
      if (m_auto) auto_tx_mode (false);
      if(onAirFreq!=m_onAirFreq0) {
        m_onAirFreq0=onAirFreq;
        auto const& message = tr ("Please choose another Tx frequency."
                                  " WSJT-X will not knowingly transmit another"
                                  " mode in the WSPR sub-band on 30m.");
#if QT_VERSION >= 0x050400
        QTimer::singleShot (0, [=] { // don't block guiUpdate
            MessageBox::warning_message (this, tr ("WSPR Guard Band"), message);
          });
#else
        MessageBox::warning_message (this, tr ("WSPR Guard Band"), message);
#endif
      }
    }

    if(m_mode=="FT8" and m_config.bFox()) {
// Don't allow Fox mode in any of the default FT8 sub-bands.
      qint32 ft8Freq[]={1840,3573,7074,10136,14074,18100,21074,24915,28074,50313,70100};
      for(int i=0; i<11; i++) {
        int kHzdiff=m_freqNominal/1000 - ft8Freq[i];
        if(qAbs(kHzdiff) < 4) {
          m_bTxTime=false;
          if (m_auto) auto_tx_mode (false);
          auto const& message = tr ("Please choose another dial frequency."
                                    " WSJT-X will not operate in Fox mode"
                                    " in the standard FT8 sub-bands.");
#if QT_VERSION >= 0x050400
          QTimer::singleShot (0, [=] {               // don't block guiUpdate
            MessageBox::warning_message (this, tr ("Fox Mode warning"), message);
          });
#else
          MessageBox::warning_message (this, tr ("Fox Mode warning"), message);
#endif
          break;
        }
      }
    }

    if (m_config.watchdog() && !m_mode.startsWith ("WSPR")
        && m_idleMinutes >= m_config.watchdog ()) {
      tx_watchdog (true);       // disable transmit
    }

    float fTR=float((ms%(1000*m_TRperiod)))/(1000*m_TRperiod);

    QString txMsg;
    if(m_ntx == 1) txMsg=ui->tx1->text();
    if(m_ntx == 2) txMsg=ui->tx2->text();
    if(m_ntx == 3) txMsg=ui->tx3->text();
    if(m_ntx == 4) txMsg=ui->tx4->text();
    if(m_ntx == 5) txMsg=ui->tx5->currentText();
    if(m_ntx == 6) txMsg=ui->tx6->text();
    if(m_ntx == 7) txMsg=ui->genMsg->text();
    if(m_ntx == 8) txMsg=ui->freeTextMsg->currentText();
    if(m_ntx == 9) txMsg=ui->nextFreeTextMsg->text();
    int msgLength=txMsg.trimmed().length();
    if(msgLength==0 and !m_tune) on_stopTxButton_clicked();

    if(g_iptt==0 and ((m_bTxTime and fTR<0.75 and msgLength>0) or m_tune)) {
      //### Allow late starts
      icw[0]=m_ncw;
      g_iptt = 1;
      setRig ();
      if(m_mode=="FT8") {
        if (m_config.bFox()) {
          if (ui->TxFreqSpinBox->value() > 900) {
            ui->TxFreqSpinBox->setValue(300);
          }
        }
        else if (m_config.bHound ()) {
          if(m_auto && !m_tune) {
            if (ui->TxFreqSpinBox->value() < 999 && m_ntx != 3) {
              int nf = (qrand() % 2000) + 1000;      // Hound randomized range: 1000-3000 Hz
              ui->TxFreqSpinBox->setValue(nf);
            }
          }
          if (m_nSentFoxRrpt==2 and m_ntx==3) {
            // move off the original Fox frequency on subsequent tries of Tx3
            int nfreq=m_nFoxFreq + 300;
            if(m_nFoxFreq>600) nfreq=m_nFoxFreq - 300;  //keep nfreq below 900 Hz
            ui->TxFreqSpinBox->setValue(nfreq);
          }
          if (m_nSentFoxRrpt == 1) {
            ++m_nSentFoxRrpt;
          }
        }
      }


// If HoldTxFreq is not checked, randomize Fox's Tx Freq
// NB: Maybe this should be done no more than once every 5 minutes or so ?
      if(m_mode=="FT8" and m_config.bFox() and !ui->cbHoldTxFreq->isChecked()) {
        int fTx = 300.0 + 300.0*double(qrand())/RAND_MAX;
        ui->TxFreqSpinBox->setValue(fTx);
      }

      setXIT (ui->TxFreqSpinBox->value ());
      Q_EMIT m_config.transceiver_ptt (true);            //Assert the PTT
      m_tx_when_ready = true;
    }
    if(!m_bTxTime and !m_tune) m_btxok=false;       //Time to stop transmitting
  }

  if(m_mode.startsWith ("WSPR") and
     ((m_ntr==1 and m_rxDone) or (m_ntr==-1 and m_nseq>tx2))) {
    if(m_monitoring) {
      m_rxDone=false;
    }
    if(m_transmitting) {
      WSPR_history(m_freqNominal,-1);
      m_bTxTime=false;                        //Time to stop a WSPR transmission
      m_btxok=false;
    }
    else if (m_ntr != -1) {
      WSPR_scheduling ();
      m_ntr=0;                                //This WSPR Rx sequence is complete
    }
  }


  // Calculate Tx tones when needed
  if((g_iptt==1 && m_iptt0==0) || m_restart) {
//----------------------------------------------------------------------
    QByteArray ba;
    QByteArray ba0;

    if(m_mode.startsWith ("WSPR")) {
      QString sdBm,msg0,msg1,msg2;
      sdBm.sprintf(" %d",m_dBm);
      m_tx=1-m_tx;
      int i2=m_config.my_callsign().indexOf("/");
      if(i2>0
         || (6 == m_config.my_grid ().size ()
             && !ui->WSPR_prefer_type_1_check_box->isChecked ())) {
        if(i2<0) {                                                 // "Type 2" WSPR message
          msg1=m_config.my_callsign() + " " + m_config.my_grid().mid(0,4) + sdBm;
        } else {
          msg1=m_config.my_callsign() + sdBm;
        }
        msg0="<" + m_config.my_callsign() + "> " + m_config.my_grid()+ sdBm;
        if(m_tx==0) msg2=msg0;
        if(m_tx==1) msg2=msg1;
      } else {
        msg2=m_config.my_callsign() + " " + m_config.my_grid().mid(0,4) + sdBm; // Normal WSPR message
      }
      ba=msg2.toLatin1();
    } else {
      if(m_config.bHound() and m_ntx!=3) {   //Hound transmits only Tx1 or Tx3
        m_ntx=1;
        ui->txrb1->setChecked(true);
      }
      if(m_ntx == 1) ba=ui->tx1->text().toLocal8Bit();
      if(m_ntx == 2) ba=ui->tx2->text().toLocal8Bit();
      if(m_ntx == 3) ba=ui->tx3->text().toLocal8Bit();
      if(m_ntx == 4) ba=ui->tx4->text().toLocal8Bit();
      if(m_ntx == 5) ba=ui->tx5->currentText().toLocal8Bit();
      if(m_ntx == 6) ba=ui->tx6->text().toLocal8Bit();
      if(m_ntx == 7) ba=ui->genMsg->text().toLocal8Bit();
      if(m_ntx == 8) ba=ui->freeTextMsg->currentText().toLocal8Bit();
      if(m_ntx == 9) ba=ui->nextFreeTextMsg->text().toLocal8Bit();
    }

    ba2msg(ba,message);
    int ichk=0;
    if (m_lastMessageSent != m_currentMessage
        || m_lastMessageType != m_currentMessageType)
      {
        m_lastMessageSent = m_currentMessage;
        m_lastMessageType = m_currentMessageType;
      }
    m_currentMessageType = 0;
    if(m_tune or m_mode=="Echo") {
      itone[0]=0;
    } else {
      if(m_mode=="ISCAT") {
        geniscat_(message, msgsent, const_cast<int *> (itone), 28, 28);
        msgsent[28]=0;
      } else {
        if(m_modeTx=="JT4") gen4_(message, &ichk , msgsent, const_cast<int *> (itone),
                                  &m_currentMessageType, 22, 22);
        if(m_modeTx=="JT9") gen9_(message, &ichk, msgsent, const_cast<int *> (itone),
                                  &m_currentMessageType, 22, 22);
        if(m_modeTx=="JT65") gen65_(message, &ichk, msgsent, const_cast<int *> (itone),
                                    &m_currentMessageType, 22, 22);
        if(m_modeTx=="QRA64") genqra64_(message, &ichk, msgsent, const_cast<int *> (itone),
                                    &m_currentMessageType, 22, 22);
        if(m_modeTx=="WSPR") genwspr_(message, msgsent, const_cast<int *> (itone),
                                    22, 22);
        if(m_modeTx=="WSPR-LF") genwspr_fsk8_(message, msgsent, const_cast<int *> (itone),
                                    22, 22);
        if(m_modeTx=="MSK144" or m_modeTx=="FT8") {
          bool bcontest=false; //ui->cbVHFcontest->isChecked();
          char MyCall[6];
          char MyGrid[6];
          strncpy(MyCall, (m_config.my_callsign()+"      ").toLatin1(),6);
          strncpy(MyGrid, (m_config.my_grid()+"      ").toLatin1(),6);
          if(m_modeTx=="MSK144") {
            genmsk144_(message, MyGrid, &ichk, &bcontest, msgsent, const_cast<int *> (itone),
                       &m_currentMessageType, 22, 6, 22);
            if(m_restart) {
              int nsym=144;
              if(itone[40]==-40) nsym=40;
              m_modulator->set_nsym(nsym);
            }
          }
          if(m_modeTx=="FT8") {
            if(m_config.bFox() and ui->tabWidget->currentIndex()==2) {
              foxTxSequencer();
            } else {
              // 0:   [000] <- this is standard set
              // 1:   [001] <- this is fox/hound
              //m_i3bit=0;
              qDebug() << "genft8" << message;
              char ft8msgbits[75 + 12]; //packed 75 bit ft8 message plus 12-bit CRC
              genft8_(message, MyGrid, &bcontest, &m_i3bit, msgsent, const_cast<char *> (ft8msgbits),
                      const_cast<int *> (itone), 22, 6, 22);
              if(m_config.bFox()) {
                QString fm = QString::fromStdString(message).trimmed();
                foxGenWaveform(0,fm);
                foxcom_.nslots=1;
                foxcom_.nfreq=ui->TxFreqSpinBox->value();
                if(m_config.split_mode()) foxcom_.nfreq = foxcom_.nfreq - m_XIT;  //Fox Tx freq
                QString foxCall=m_config.my_callsign() + "         ";
                strncpy(&foxcom_.mycall[0], foxCall.toLatin1(),12);   //Copy Fox callsign into foxcom_
                foxgen_();
              }
            }
          }
        }
        msgsent[22]=0;
      }
    }

    m_currentMessage = QString::fromLatin1(msgsent);
    m_bCallingCQ = CALLING == m_QSOProgress
      || m_currentMessage.contains (QRegularExpression {"^(CQ|QRZ) "});
    if(m_mode=="FT8") {
      if(m_bCallingCQ && ui->cbFirst->isVisible () && ui->cbFirst->isChecked ()) {
        ui->cbFirst->setStyleSheet("QCheckBox{color:red}");
      } else {
        ui->cbFirst->setStyleSheet("");
      }
    }

    if (m_tune) {
      m_currentMessage = "TUNE";
      m_currentMessageType = -1;
    }
    if(m_restart) {
      write_transmit_entry ("ALL.TXT");
      if (m_config.TX_messages ()) {
        ui->decodedTextBrowser2->displayTransmittedText(m_currentMessage,m_modeTx,
                     ui->TxFreqSpinBox->value(),m_config.color_ReceivedMsg(),m_bFastMode);
        }
    }

    auto t2 = QDateTime::currentDateTimeUtc ().toString ("hhmm");
    icw[0] = 0;
    auto msg_parts = m_currentMessage.split (' ', QString::SkipEmptyParts);
    if (msg_parts.size () > 2) {
      // clean up short code forms
      msg_parts[0].remove (QChar {'<'});
      msg_parts[1].remove (QChar {'>'});
    }
    auto is_73 = m_QSOProgress >= ROGER_REPORT
      && message_is_73 (m_currentMessageType, msg_parts);
    m_sentFirst73 = is_73
      && !message_is_73 (m_lastMessageType, m_lastMessageSent.split (' ', QString::SkipEmptyParts));
    if (m_sentFirst73) {
      m_qsoStop=t2;
      if(m_config.id_after_73 ()) {
        icw[0] = m_ncw;
      }
      if (m_config.prompt_to_log () && !m_tune) {
        logQSOTimer.start (0);
      }
    }

    bool b=(m_mode=="FT8") and ui->cbAutoSeq->isChecked() and ui->cbFirst->isChecked();
    if(is_73 and (m_config.disable_TX_on_73() or b)) {
      auto_tx_mode (false);
      if(b) {
        m_ntx=6;
        ui->txrb6->setChecked(true);
        m_QSOProgress = CALLING;
      }
    }

    if(m_config.id_interval () >0) {
      int nmin=(m_sec0-m_secID)/60;
      if(m_sec0<m_secID) nmin=m_config.id_interval();
      if(nmin >= m_config.id_interval()) {
        icw[0]=m_ncw;
        m_secID=m_sec0;
      }
    }

    if ((m_currentMessageType < 6 || 7 == m_currentMessageType)
        && msg_parts.length() >= 3
        && (msg_parts[1] == m_config.my_callsign () ||
            msg_parts[1] == m_baseCall))
    {
      int i1;
      bool ok;
      i1 = msg_parts[2].toInt(&ok);
      if(ok and i1>=-50 and i1<50)
      {
        m_rptSent = msg_parts[2];
        m_qsoStart = t2;
      } else {
        if (msg_parts[2].mid (0, 1) == "R")
        {
          i1 = msg_parts[2].mid (1).toInt (&ok);
          if (ok and i1 >= -50 and i1 < 50)
          {
            m_rptSent = msg_parts[2].mid (1);
            m_qsoStart = t2;
          }
        }
      }
    }
    m_restart=false;
//----------------------------------------------------------------------
  } else {
    if (!m_auto && m_sentFirst73)
    {
      m_sentFirst73 = false;
      if (1 == ui->tabWidget->currentIndex())
      {
        ui->genMsg->setText(ui->tx6->text());
        m_ntx=7;
        m_QSOProgress = CALLING;
        m_gen_message_is_cq = true;
        ui->rbGenMsg->setChecked(true);
      } else {
//JHT 11/29/2015        m_ntx=6;
//        ui->txrb6->setChecked(true);
      }
    }
  }

  if (g_iptt == 1 && m_iptt0 == 0)
    {
      auto const& current_message = QString::fromLatin1 (msgsent);
      if(m_config.watchdog () && !m_mode.startsWith ("WSPR")
         && current_message != m_msgSent0) {
        tx_watchdog (false);  // in case we are auto sequencing
        m_msgSent0 = current_message;
      }

      if(!m_tune) {
        write_transmit_entry ("ALL.TXT");
      }

      if (m_config.TX_messages () && !m_tune && !m_config.bFox()) {
        ui->decodedTextBrowser2->displayTransmittedText(current_message, m_modeTx,
              ui->TxFreqSpinBox->value(),m_config.color_ReceivedMsg(),m_bFastMode);
      }

      switch (m_ntx)
        {
        case 1: m_QSOProgress = REPLYING; break;
        case 2: m_QSOProgress = REPORT; break;
        case 3: m_QSOProgress = ROGER_REPORT; break;
        case 4: m_QSOProgress = ROGERS; break;
        case 5: m_QSOProgress = SIGNOFF; break;
        case 6: m_QSOProgress = CALLING; break;
        default: break;             // determined elsewhere
        }

      // TODO: jsherer - perhaps an on_transmitting signal?
      m_lastTxTime = QDateTime::currentDateTimeUtc();

      m_transmitting = true;
      transmitDisplay (true);
      statusUpdate ();
    }

  if(!m_btxok && m_btxok0 && g_iptt==1) stopTx();

  if(m_startAnother) {
    if(m_mode=="MSK144") {
      m_wait++;
    }
    if(m_mode!="MSK144" or m_wait>=4) {
      m_wait=0;
      m_startAnother=false;
      on_actionOpen_next_in_directory_triggered();
    }
  }

  //Once per second:
  if(nsec != m_sec0) {
    if(m_freqNominal!=0 and m_freqNominal<50000000 and m_config.enable_VHF_features()) {
      if(!m_bVHFwarned) vhfWarning();
    } else {
      m_bVHFwarned=false;
    }
    // if(m_config.bFox()) {
    //   if(m_config.my_callsign()=="K1JT" or m_config.my_callsign()=="K9AN" or
    //      m_config.my_callsign()=="G4WJS" or m_config.my_callsign().contains("KH7Z")) {
    //     ui->sbNslots->setMaximum(5);
    //     m_Nslots=ui->sbNslots->value();
    //     ui->sbNslots->setEnabled(true);
    //   } else {
    //     ui->sbNslots->setMaximum(1);
    //     m_Nslots=1;
    //     ui->sbNslots->setEnabled(false);
    //   }
    // }

    if(m_config.bHound()) {
      m_bWarnedHound=false;
      qint32 tHound=QDateTime::currentMSecsSinceEpoch()/1000 - m_tAutoOn;
      //To keep calling Fox, Hound must reactivate Enable Tx at least once every 2 minutes
      if(tHound >= 120 and m_ntx==1) auto_tx_mode(false);
    }

    if(m_auto and m_mode=="Echo" and m_bEchoTxOK) {
      progressBar.setMaximum(6);
      progressBar.setValue(int(m_s6));
    }

    if(m_mode!="Echo") {
      if(m_monitoring or m_transmitting) {
        progressBar.setMaximum(m_TRperiod);
        int isec=int(fmod(tsec,m_TRperiod));
        progressBar.setValue(isec);
      } else {
        progressBar.setValue(0);
      }
    }

    astroUpdate ();

    if(m_transmitting) {
      char s[41];
      if(m_config.bFox() and ui->tabWidget->currentIndex()==2) {
        sprintf(s,"Tx:  %d Slots",foxcom_.nslots);
      } else {
        auto dt = DecodedText(msgsent);
        sprintf(s,"Tx: %s", dt.message().toLocal8Bit().mid(0, 41).data());
      }
      m_nsendingsh=0;
      if(s[4]==64) m_nsendingsh=1;
      if(m_nsendingsh==1 or m_currentMessageType==7) {
        tx_status_label.setStyleSheet("QLabel{background-color: #66ffff}");
      } else if(m_nsendingsh==-1 or m_currentMessageType==6) {
        tx_status_label.setStyleSheet("QLabel{background-color: #ffccff}");
      } else {
        tx_status_label.setStyleSheet("QLabel{background-color: #ffff33}");
      }
      if(m_tune) {
        tx_status_label.setText("Tx: TUNE");
      } else {
        if(m_mode=="Echo") {
          tx_status_label.setText("Tx: ECHO");
        } else {
          s[40]=0;
          QString t{QString::fromLatin1(s)};
          if(m_config.bFox() and ui->tabWidget->currentIndex()==2 and foxcom_.nslots==1) {
              t=m_fm1.trimmed();
          }
          tx_status_label.setText(t.trimmed());
        }
      }
    } else if(m_monitoring) {
      if (!m_tx_watchdog) {
        tx_status_label.setStyleSheet("QLabel{background-color: #00ff00}");
        QString t;
        t="Receiving";
        if(m_mode=="MSK144") {
          int npct=int(100.0*m_fCPUmskrtd/0.298667);
          if(npct>90) tx_status_label.setStyleSheet("QLabel{background-color: #ff0000}");
          t.sprintf("Receiving   %2d%%",npct);
        }
        tx_status_label.setText (t);
      }
      transmitDisplay(false);
    } else if (!m_diskData && !m_tx_watchdog) {
      tx_status_label.setStyleSheet("");
      tx_status_label.setText("");
    }

    QDateTime t = QDateTime::currentDateTimeUtc();
    QString utc = t.date().toString("yyyy MMM dd") + "\n " +
      t.time().toString() + " ";
    ui->labUTC->setText(utc);

    auto delta = t.secsTo(m_nextBeacon);
    auto beacon = ui->beaconButton->isChecked() ? delta > 0 ? QString("%1 s").arg(delta) : "queued!" : m_nextBeaconPaused ? "paused" : "disabled";
    ui->labBeacon->setText(QString("Next Beacon: %1").arg(beacon));

    auto callLabel = m_config.my_callsign();
    if(m_config.use_dynamic_grid() && !m_config.my_grid().isEmpty()){
        callLabel = QString("%1 - %2").arg(callLabel).arg(m_config.my_grid());
    }
    ui->labCallsign->setText(callLabel);

    if(!m_monitoring and !m_diskData) {
      ui->signal_meter_widget->setValue(0,0);
    }

    m_sec0=nsec;

    // once per period
    if(m_sec0 % m_TRperiod == 0){
        tryBandHop();
    }

    // once per period/3
    bool forceDirty = false;
    if(m_sec0 % (m_TRperiod/3) == 0){
        // force rx dirty three times per period
        forceDirty = true;
    }

    // once per second...

    // update the dial frequency once per second..
    displayDialFrequency();

    // process all received activity...
    processActivity(forceDirty);

    // process outgoing tx queue...
    processTxQueue();

    // once processed, lets update the display...
    displayActivity(forceDirty);
  }

  // once per 100ms
  displayTransmit();

  m_iptt0=g_iptt;
  m_btxok0=m_btxok;
}               //End of guiUpdate


void MainWindow::startTx()
{
  if(!prepareNextMessageFrame()){
    return;
  }

  m_ntx=9;
  m_QSOProgress = CALLING;
  set_dateTimeQSO(-1);
  ui->rbNextFreeTextMsg->setChecked(true);
  if (m_transmitting) m_restart=true;

  // detect if we're currently in a possible transmit cycle...and if so, wait until the next one if we're more than 2 seconds in...
  QDateTime now {QDateTime::currentDateTimeUtc()};
  int s=now.addSecs(-2).time().second();
  int n=s % (2*m_TRperiod);
  if((n <= m_TRperiod && m_txFirst) || (n > m_TRperiod && !m_txFirst)){
    ui->txFirstCheckBox->setChecked(!m_txFirst);
  }

  // hack the auto button to kick off the transmit
  if(!ui->autoButton->isChecked()){
    ui->autoButton->setEnabled(true);
    ui->autoButton->click();
    ui->autoButton->setEnabled(false);
  }

  // disallow editing of the text while transmitting
  ui->extFreeTextMsgEdit->setReadOnly(true);
  update_dynamic_property(ui->extFreeTextMsgEdit, "transmitting", true);
}

void MainWindow::startTx2()
{
  if (!m_modulator->isActive ()) { // TODO - not thread safe
    double fSpread=0.0;
    double snr=99.0;
    QString t=ui->tx5->currentText();
    if(t.mid(0,1)=="#") fSpread=t.mid(1,5).toDouble();
    m_modulator->setSpread(fSpread); // TODO - not thread safe
    t=ui->tx6->text();
    if(t.mid(0,1)=="#") snr=t.mid(1,5).toDouble();
    if(snr>0.0 or snr < -50.0) snr=99.0;
    transmit (snr);
    ui->signal_meter_widget->setValue(0,0);
    if(m_mode=="Echo" and !m_tune) m_bTransmittedEcho=true;

    if(m_mode.startsWith ("WSPR") and !m_tune) {
      if (m_config.TX_messages ()) {
        t = " Transmitting " + m_mode + " ----------------------- " +
          m_config.bands ()->find (m_freqNominal);
        t=WSPR_hhmm(0) + ' ' + t.rightJustified (66, '-');
        ui->decodedTextBrowser->appendText(t);
      }
      write_transmit_entry ("ALL_WSPR.TXT");
    }
  }
}

void MainWindow::continueTx()
{
    ui->txFirstCheckBox->setChecked(!m_txFirst);
}

void MainWindow::stopTx()
{
  Q_EMIT endTransmitMessage ();
  m_btxok = false;
  m_transmitting = false;
  g_iptt=0;
  if (!m_tx_watchdog) {
    tx_status_label.setStyleSheet("");
    tx_status_label.setText("");
  }

  if(prepareNextMessageFrame()){
      continueTx();
  } else {
      // TODO: jsherer - split this up...
      ui->extFreeTextMsgEdit->setReadOnly(false);
      update_dynamic_property(ui->extFreeTextMsgEdit, "transmitting", false);
      on_stopTxButton_clicked();
      tryRestoreFreqOffset();
  }

  ptt0Timer.start(200);                       //end-of-transmission sequencer delay
  monitor (true);
  statusUpdate ();
}

void MainWindow::stopTx2()
{
  Q_EMIT m_config.transceiver_ptt (false);      //Lower PTT
  if (m_mode == "JT9" && m_bFast9
      && ui->cbAutoSeq->isVisible () && ui->cbAutoSeq->isChecked()
      && m_ntx == 5 && m_nTx73 >= 5) {
    on_stopTxButton_clicked ();
    m_nTx73 = 0;
  }
  if(m_mode.startsWith ("WSPR") and m_ntr==-1 and !m_tuneup) {
    m_wideGraph->setWSPRtransmitted();
    WSPR_scheduling ();
    m_ntr=0;
  }
  auto dt = DecodedText(m_currentMessage.trimmed());
  last_tx_label.setText("Last Tx: " + dt.message()); //m_currentMessage.trimmed());
//###  if(m_mode=="FT8" and m_config.bHound()) auto_tx_mode(false); ###
}

void MainWindow::ba2msg(QByteArray ba, char message[])             //ba2msg()
{
  int iz=ba.length();
  for(int i=0;i<28; i++) {
    if(i<iz) {
      message[i]=ba[i];
    } else {
      message[i]=32;
    }
  }
  message[28]=0;
}

void MainWindow::on_txFirstCheckBox_stateChanged(int nstate)        //TxFirst
{
  m_txFirst = (nstate==2);
}

void MainWindow::set_dateTimeQSO(int m_ntx)
{
    // m_ntx = -1 resets to default time
    // Our QSO start time can be fairly well determined from Tx 2 and Tx 3 -- the grid reports
    // If we CQ'd and sending sigrpt then 2 minutes ago n=2
    // If we're on msg 3 then 3 minutes ago n=3 -- might have sat on msg1 for a while
    // If we've already set our time on just return.
    // This should mean that Tx2 or Tx3 has been repeated so don't update the start time
    // We reset it in several places
    if (m_ntx == -1) { // we use a default date to detect change
      m_dateTimeQSOOn = QDateTime {};
    }
    else if (m_dateTimeQSOOn.isValid ()) {
        return;
    }
    else { // we also take of m_TRperiod/2 to allow for late clicks
      auto now = QDateTime::currentDateTimeUtc();
      m_dateTimeQSOOn = now.addSecs (-(m_ntx - 2) * m_TRperiod - (now.time ().second () % m_TRperiod));
    }
}

void MainWindow::set_ntx(int n)                                   //set_ntx()
{
  m_ntx=n;
}

void MainWindow::on_txrb1_toggled (bool status)
{
  if (status) {
    if (ui->tx1->isEnabled ()) {
      m_ntx = 1;
      set_dateTimeQSO (-1); // we reset here as tx2/tx3 is used for start times
    }
    else {
      QTimer::singleShot (0, ui->txrb2, SLOT (click ()));
    }
  }
}

void MainWindow::on_txrb1_doubleClicked ()
{
  if(m_mode=="FT8" and m_config.bHound()) return;
  // skip Tx1, only allowed if not a type 2 compound callsign
  auto const& my_callsign = m_config.my_callsign ();
  auto is_compound = my_callsign != m_baseCall;
  ui->tx1->setEnabled ((is_compound && shortList (my_callsign)) || !ui->tx1->isEnabled ());
  if (!ui->tx1->isEnabled ()) {
    // leave time for clicks to complete before setting txrb2
    QTimer::singleShot (500, ui->txrb2, SLOT (click ()));
  }
}

void MainWindow::on_txrb2_toggled (bool status)
{
  // Tx 2 means we already have CQ'd so good reference
  if (status) {
    m_ntx = 2;
    set_dateTimeQSO (m_ntx);
  }
}

void MainWindow::on_txrb3_toggled(bool status)
{
  // Tx 3 means we should havel already have done Tx 1 so good reference
  if (status) {
    m_ntx=3;
    set_dateTimeQSO(m_ntx);
  }
}

void MainWindow::on_txrb4_toggled (bool status)
{
  if (status) {
    m_ntx=4;
  }
}

void MainWindow::on_txrb4_doubleClicked ()
{
  // RR73 only allowed if not a type 2 compound callsign
  auto const& my_callsign = m_config.my_callsign ();
  auto is_compound = my_callsign != m_baseCall;
  m_send_RR73 = !((is_compound && !shortList (my_callsign)) || m_send_RR73);
  genStdMsgs (m_rpt);
}

void MainWindow::on_txrb5_toggled (bool status)
{
  if (status) {
    m_ntx = 5;
  }
}

void MainWindow::on_txrb5_doubleClicked ()
{
  genStdMsgs (m_rpt, true);
}

void MainWindow::on_txrb6_toggled(bool status)
{
  if (status) {
    m_ntx=6;
    if (ui->txrb6->text().contains (QRegularExpression {"^(CQ|QRZ) "})) set_dateTimeQSO(-1);
  }
}

void MainWindow::on_txb1_clicked()
{
  if (ui->tx1->isEnabled ()) {
    m_ntx=1;
    m_QSOProgress = REPLYING;
    ui->txrb1->setChecked(true);
    if (m_transmitting) m_restart=true;
  }
  else {
    on_txb2_clicked ();
  }
}

void MainWindow::on_txb1_doubleClicked()
{
  if(m_mode=="FT8" and m_config.bHound()) return;
  // skip Tx1, only allowed if not a type 1 compound callsign
  auto const& my_callsign = m_config.my_callsign ();
  auto is_compound = my_callsign != m_baseCall;
  ui->tx1->setEnabled ((is_compound && shortList (my_callsign)) || !ui->tx1->isEnabled ());
}

void MainWindow::on_txb2_clicked()
{
    m_ntx=2;
    m_QSOProgress = REPORT;
    ui->txrb2->setChecked(true);
    if (m_transmitting) m_restart=true;
}

void MainWindow::on_txb3_clicked()
{
    m_ntx=3;
    m_QSOProgress = ROGER_REPORT;
    ui->txrb3->setChecked(true);
    if (m_transmitting) m_restart=true;
}

void MainWindow::on_txb4_clicked()
{
    m_ntx=4;
    m_QSOProgress = ROGERS;
    ui->txrb4->setChecked(true);
    if (m_transmitting) m_restart=true;
}

void MainWindow::on_txb4_doubleClicked()
{
  // RR73 only allowed if not a type 2 compound callsign
  auto const& my_callsign = m_config.my_callsign ();
  auto is_compound = my_callsign != m_baseCall;
  m_send_RR73 = !((is_compound && !shortList (my_callsign)) || m_send_RR73);
  genStdMsgs (m_rpt);
}

void MainWindow::on_txb5_clicked()
{
    m_ntx=5;
    m_QSOProgress = SIGNOFF;
    ui->txrb5->setChecked(true);
    if (m_transmitting) m_restart=true;
}

void MainWindow::on_txb5_doubleClicked()
{
  genStdMsgs (m_rpt, true);
}

void MainWindow::on_txb6_clicked()
{
    m_ntx=6;
    m_QSOProgress = CALLING;
    set_dateTimeQSO(-1);
    ui->txrb6->setChecked(true);
    if (m_transmitting) m_restart=true;
}

void MainWindow::doubleClickOnCall2(Qt::KeyboardModifiers modifiers)
{
  set_dateTimeQSO(-1); // reset our QSO start time
  m_decodedText2=true;
  doubleClickOnCall(modifiers);
  m_decodedText2=false;
}

void MainWindow::doubleClickOnCall(Qt::KeyboardModifiers modifiers)
{
  QTextCursor cursor;
  if(m_mode=="ISCAT") {
    MessageBox::information_message (this,
        "Double-click not presently implemented for ISCAT mode");
  }
  if(m_decodedText2) {
    cursor=ui->decodedTextBrowser->textCursor();
  } else {
    cursor=ui->decodedTextBrowser2->textCursor();
  }

  if(modifiers==(Qt::ShiftModifier + Qt::ControlModifier + Qt::AltModifier)) {
    cursor.setPosition(0);
  } else {
    cursor.setPosition(cursor.selectionStart());
  }

  if(m_config.bFox() and m_decodedText2) {
    if(m_houndQueue.count()<10 and m_nSortedHounds>0) {
      QString t=cursor.block().text();
      selectHound(t);
    }
    return;
  }
  DecodedText message {cursor.block().text(), ("MSK144" == m_mode || "FT8" == m_mode) &&
        ui->cbVHFcontest->isChecked(), m_config.my_grid ()};
  m_bDoubleClicked = true;
  processMessage (message, modifiers);
}

void MainWindow::processMessage (DecodedText const& message, Qt::KeyboardModifiers modifiers)
{
  // decode keyboard modifiers we are interested in
  auto shift = modifiers.testFlag (Qt::ShiftModifier);
  auto ctrl = modifiers.testFlag (Qt::ControlModifier);
  // auto alt = modifiers.testFlag (Qt::AltModifier);

  // basic mode sanity checks
  auto const& parts = message.string ().split (' ', QString::SkipEmptyParts);
  if (parts.size () < 5) return;
  auto const& mode = parts.at (4).left (1);
  if (("JT9+JT65" == m_mode && !("@" == mode || "#" == mode))
      || ("JT65" == m_mode && mode != "#")
      || ("JT9" == m_mode && mode != "@")
      || ("MSK144" == m_mode && !("&" == mode || "^" == mode))
      || ("QRA64" == m_mode && mode.left (1) != ":")) {
    return;
  }

  //Skip the rest if no decoded text extracted
  int frequency = message.frequencyOffset();
  if (message.isTX()) {
    if (!m_config.enable_VHF_features()) {
      if(!shift) ui->RxFreqSpinBox->setValue(frequency); //Set Rx freq
      if((ctrl or shift) and !ui->cbHoldTxFreq->isChecked ()) {
        ui->TxFreqSpinBox->setValue(frequency); //Set Tx freq
      }
    }
    return;
  }

  // check for CQ with listening frequency
  if (parts.size () >= 7
      && (m_bFastMode || m_mode=="FT8")
      && "CQ" == parts[5]
      && m_config.is_transceiver_online ()) {
    bool ok;
    auto kHz = parts[6].toUInt (&ok);
    if (ok && kHz >= 10 && 3 == parts[6].size ()) {
      // QSY Freq for answering CQ nnn
      setRig (m_freqNominal / 1000000 * 1000000 + 1000 * kHz);
      ui->decodedTextBrowser2->displayQSY (QString {"QSY %1"}.arg (m_freqNominal / 1e6, 7, 'f', 3));
    }
  }

  int nmod = message.timeInSeconds () % (2*m_TRperiod);
  m_txFirst=(nmod!=0);
  ui->txFirstCheckBox->setChecked(m_txFirst);

  auto const& message_words = message.messageWords ();
  if (message_words.size () < 2) return;

  QString hiscall;
  QString hisgrid;
  message.deCallAndGrid(/*out*/hiscall,hisgrid);
  int nWarn=0;
  QString warnMsg;

  if(m_mode=="MSK144" and message.string().indexOf(hiscall+" R ")>0 and
     !ui->cbVHFcontest->isChecked()) {
    warnMsg=tr("Should you be operating in NA VHF Contest mode?");
    nWarn=1;
  }
  if((m_mode=="FT8" or m_mode=="MSK144") and hisgrid.contains(grid_regexp) and
     m_rigState.frequency()>50000000 and !m_bCheckedContest) {
    double utch=0.0;
    int nAz,nEl,nDmiles,nDkm,nHotAz,nHotABetter;
    azdist_(const_cast <char *> (m_config.my_grid().toLatin1().constData()),
            const_cast <char *> (hisgrid.toLatin1().constData()),&utch,
            &nAz,&nEl,&nDmiles,&nDkm,&nHotAz,&nHotABetter,6,6);
    if(nDkm>10000) {
      warnMsg=tr("Locator in decoded message seems to imply\n"
             "a distance greater than 10,000 km. Should\n"
             "you be operating in NA VHF Contest mode?");
      nWarn=2;
    }
  }

  if(nWarn>0) {
    QMessageBox msgBox;
    msgBox.setWindowTitle("Contest mode?");
    msgBox.setText(warnMsg);
    msgBox.setStandardButtons(QMessageBox::Yes);
    msgBox.addButton(QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    if(msgBox.exec() == QMessageBox::Yes){
      ui->cbVHFcontest->setChecked(true);
      if(nWarn==2) {
        on_DecodeButton_clicked (true);
      }
    } else {
      ui->cbVHFcontest->setChecked(false);
    }
    m_bCheckedContest=true;
  }

  auto is_73 = message_words.filter (QRegularExpression {"^(73|RR73)$"}).size ();
  if (!is_73 and !message.isStandardMessage() and (nWarn==0)) {
    qDebug () << "Not processing message - hiscall:" << hiscall << "hisgrid:" << hisgrid;
    return;
  }
  // only allow automatic mode changes between JT9 and JT65, and when not transmitting
  if (!m_transmitting and m_mode == "JT9+JT65") {
    if (message.isJT9())
      {
        m_modeTx="JT9";
        ui->pbTxMode->setText("Tx JT9  @");
        m_wideGraph->setModeTx(m_modeTx);
      } else if (message.isJT65()) {
      m_modeTx="JT65";
      ui->pbTxMode->setText("Tx JT65  #");
      m_wideGraph->setModeTx(m_modeTx);
    }
  } else if ((message.isJT9 () and m_modeTx != "JT9" and m_mode != "JT4") or
             (message.isJT65 () and m_modeTx != "JT65" and m_mode != "JT4")) {
    // if we are not allowing mode change then don't process decode
    return;
  }

  QString firstcall = message.call();
  if(firstcall.length()==5 and firstcall.mid(0,3)=="CQ ") firstcall="CQ";
  if(!m_bFastMode and (!m_config.enable_VHF_features() or m_mode=="FT8")) {
    // Don't change Tx freq if in a fast mode, or VHF features enabled; also not if a
    // station is calling me, unless CTRL or SHIFT is held down.
    if ((Radio::is_callsign (firstcall)
         && firstcall != m_config.my_callsign () && firstcall != m_baseCall
         && firstcall != "DE")
        || "CQ" == firstcall || "QRZ" == firstcall || ctrl || shift) {
      if ((!m_config.bHound () || m_mode != "FT8")
          && (!ui->cbHoldTxFreq->isChecked () || shift || ctrl)) {
        ui->TxFreqSpinBox->setValue(frequency);
      }
      if(m_mode != "JT4" && m_mode != "JT65" && !m_mode.startsWith ("JT9") &&
         m_mode != "QRA64" && m_mode!="FT8") {
        return;
      }
    }
  }

  // prior DX call (possible QSO partner)
  auto qso_partner_base_call = Radio::base_callsign (ui->dxCallEntry-> text ());
  auto base_call = Radio::base_callsign (hiscall);

// Determine appropriate response to received message
  auto dtext = " " + message.string () + " ";
  int gen_msg {0};
  if(dtext.contains (" " + m_baseCall + " ")
     || dtext.contains ("<" + m_baseCall + " ")
     || dtext.contains ("/" + m_baseCall + " ")
     || dtext.contains (" " + m_baseCall + "/")
     || (firstcall == "DE" /*&& ((t4.size () > 7 && t4.at(7) != "73") || t4.size () <= 7)*/))
    {
      if (message_words.size () > 3   // enough fields for a normal message
          && (message_words.at (1).contains (m_baseCall) || "DE" == message_words.at (1))
          && message_words.at (2).contains (qso_partner_base_call)
          && !message_words.at (3).contains (grid_regexp)) // but no grid on end of msg
        {
          QString r=message_words.at (3);
          if(m_QSOProgress >= ROGER_REPORT && (r=="RRR" || r.toInt()==73 || "RR73" == r)) {
            if(ui->tabWidget->currentIndex()==1) {
              gen_msg = 5;
              if (ui->rbGenMsg->isChecked ()) m_ntx=7;
              m_gen_message_is_cq = false;
            }
            else {
              m_ntx=5;
              ui->txrb5->setChecked(true);
            }
            m_QSOProgress = SIGNOFF;
          } else if((m_QSOProgress >= REPORT
                     || (m_QSOProgress >= REPLYING && (m_mode=="MSK144" or m_mode=="FT8")
                         && ui->cbVHFcontest->isChecked())) && r.mid(0,1)=="R") {
            m_ntx=4;
            m_QSOProgress = ROGERS;
            ui->txrb4->setChecked(true);
            if(ui->tabWidget->currentIndex()==1) {
              gen_msg = 4;
              m_ntx=7;
              m_gen_message_is_cq = false;
            }
          } else if(m_QSOProgress >= CALLING && r.toInt()>=-50 && r.toInt()<=49) {
            m_ntx=3;
            m_QSOProgress = ROGER_REPORT;
            ui->txrb3->setChecked(true);
            if(ui->tabWidget->currentIndex()==1) {
              gen_msg = 3;
              m_ntx=7;
              m_gen_message_is_cq = false;
            }
          }
          else {                // nothing for us
            return;
          }
        }
      else if (m_QSOProgress >= ROGERS
               && message_words.size () > 2 && message_words.at (1).contains (m_baseCall) && message_words.at (2) == "73") {
        // 73 back to compound call holder
        if(ui->tabWidget->currentIndex()==1) {
          gen_msg = 5;
          if (ui->rbGenMsg->isChecked ()) m_ntx=7;
          m_gen_message_is_cq = false;
        }
        else {
          m_ntx=5;
          ui->txrb5->setChecked(true);
        }
        m_QSOProgress = SIGNOFF;
      }
      else if (!(m_bAutoReply && m_QSOProgress > CALLING)) {
        if ((message_words.size () > 4 && message_words.at (1).contains (m_baseCall) && message_words.at (4) == "OOO")
            || ((m_mode=="MSK144" or m_mode=="FT8") && ui->cbVHFcontest->isChecked())) {
          // EME short code report or MSK144/FT8 contest mode reply, send back Tx3
          m_ntx = 3;
          m_QSOProgress = ROGER_REPORT;
          ui->txrb3->setChecked (true);
          if (ui->tabWidget->currentIndex () == 1) {
            gen_msg = 3;
            m_ntx = 7;
            m_gen_message_is_cq = false;
          }
        } else if (!is_73) {    // don't respond to sign off messages
          m_ntx=2;
          m_QSOProgress = REPORT;
          ui->txrb2->setChecked(true);
          if(ui->tabWidget->currentIndex()==1) {
            gen_msg = 2;
            m_ntx=7;
            m_gen_message_is_cq = false;
          }

          if (m_bDoubleClickAfterCQnnn and m_transmitting) {
            on_stopTxButton_clicked();
            TxAgainTimer.start(1500);
          }
          m_bDoubleClickAfterCQnnn=false;
        }
        else {
          return;               // nothing we need to respond to
        }
      }
      else {                  // nothing for us
        return;
      }
    }
  else if (firstcall == "DE" && message_words.size () > 3 && message_words.at (3) == "73") {
    if (m_QSOProgress >= ROGERS && base_call == qso_partner_base_call && m_currentMessageType) {
      // 73 back to compound call holder
      if(ui->tabWidget->currentIndex()==1) {
        gen_msg = 5;
        m_ntx=7;
        m_gen_message_is_cq = false;
      }
      else {
        m_ntx=5;
        ui->txrb5->setChecked(true);
      }
      m_QSOProgress = SIGNOFF;
    }
    else {
      // treat like a CQ/QRZ
      if (ui->tx1->isEnabled ()) {
        m_ntx = 1;
        m_QSOProgress = REPLYING;
        ui->txrb1->setChecked (true);
      }
      else {
        m_ntx = 2;
        m_QSOProgress = REPORT;
        ui->txrb2->setChecked (true);
      }
      if(ui->tabWidget->currentIndex()==1) {
        gen_msg = 1;
        m_ntx=7;
        m_gen_message_is_cq = false;
      }
    }
  }
  else if (is_73 && !message.isStandardMessage ()) {
    if(ui->tabWidget->currentIndex()==1) {
      gen_msg = 5;
      if (ui->rbGenMsg->isChecked ()) m_ntx=7;
      m_gen_message_is_cq = false;
    }
    else {
      m_ntx=5;
      ui->txrb5->setChecked(true);
    }
    m_QSOProgress = SIGNOFF;
  }
  else // just work them
    {
      if (ui->tx1->isEnabled ()) {
        m_ntx = 1;
        m_QSOProgress = REPLYING;
        ui->txrb1->setChecked (true);
      }
      else {
        m_ntx = 2;
        m_QSOProgress = REPORT;
        ui->txrb2->setChecked (true);
      }
      if (1 == ui->tabWidget->currentIndex ()) {
        gen_msg = m_ntx;
        m_ntx=7;
        m_gen_message_is_cq = false;
      }
    }

  // if we get here then we are reacting to the message
  if (m_bAutoReply) m_bCallingCQ = CALLING == m_QSOProgress;
  if (ui->RxFreqSpinBox->isEnabled () and m_mode != "MSK144" and !shift) {
    ui->RxFreqSpinBox->setValue (frequency);    //Set Rx freq
  }

  QString s1 = m_QSOText.trimmed ();
  QString s2 = message.string ().trimmed();
  if (s1!=s2 and !message.isTX()) {
    if (!s2.contains(m_baseCall) or m_mode=="MSK144") {  // Taken care of elsewhere if for_us and slow mode
      ui->decodedTextBrowser2->displayDecodedText(message, m_baseCall,
          false, m_logBook,m_config.color_CQ(), m_config.color_MyCall(),
          m_config.color_DXCC(),m_config.color_NewCall(),m_config.ppfx());
    }
    m_QSOText = s2;
  }

  if (Radio::is_callsign (hiscall)
      && (base_call != qso_partner_base_call || base_call != hiscall)) {
    if (qso_partner_base_call != base_call) {
      // clear the DX grid if the base call of his call is different
      // from the current DX call
      ui->dxGridEntry->clear ();
    }
    // his base call different or his call more qualified
    // i.e. compound version of same base call
    ui->dxCallEntry->setText (hiscall);
  }
  if (hisgrid.contains (grid_regexp)) {
    if(ui->dxGridEntry->text().mid(0,4) != hisgrid) ui->dxGridEntry->setText(hisgrid);
   }
  if (!ui->dxGridEntry->text ().size ())
    lookup();
  m_hisGrid = ui->dxGridEntry->text();

  QString rpt = message.report();
  int n=rpt.toInt();
  if(m_mode=="MSK144" and m_bShMsgs) {
    int n=rpt.toInt();
    if(n<=-2) n=-3;
    if(n>=-1 and n<=1) n=0;
    if(n>=2 and n<=4) n=3;
    if(n>=5 and n<=7) n=6;
    if(n>=8 and n<=11) n=10;
    if(n>=12 and n<=14) n=13;
    if(n>=15) n=16;
    rpt=QString::number(n);
  }

  ui->rptSpinBox->setValue(n);
  if (!m_nTx73) {      // Don't genStdMsgs if we're already sending 73.
    genStdMsgs(rpt);
    if (gen_msg) {
      switch (gen_msg) {
      case 1: ui->genMsg->setText (ui->tx1->text ()); break;
      case 2: ui->genMsg->setText (ui->tx2->text ()); break;
      case 3: ui->genMsg->setText (ui->tx3->text ()); break;
      case 4: ui->genMsg->setText (ui->tx4->text ()); break;
      case 5: ui->genMsg->setText (ui->tx5->currentText ()); break;
      }
      if (gen_msg != 5) {        // allow user to pre-select a free message
        ui->rbGenMsg->setChecked (true);
      }
    }
  }

  if(m_transmitting) m_restart=true;
  if (ui->cbAutoSeq->isVisible () && ui->cbAutoSeq->isChecked () && !m_bDoubleClicked) return;
  if(m_config.quick_call()) auto_tx_mode(true);
  m_bDoubleClicked=false;
}

void MainWindow::genCQMsg ()
{
  if(m_config.my_callsign().size () && m_config.my_grid().size ()) {
    auto const& grid = m_config.my_callsign () != m_baseCall && shortList (m_config.my_callsign ()) ? QString {} : m_config.my_grid ();
    if (ui->cbCQTx->isEnabled () && ui->cbCQTx->isVisible () && ui->cbCQTx->isChecked ()) {
      msgtype (QString {"CQ %1 %2 %3"}
               .arg (m_freqNominal / 1000 - m_freqNominal / 1000000 * 1000, 3, 10, QChar {'0'})
               .arg (m_config.my_callsign())
               .arg (grid.left (4)),
               ui->tx6);
    } else {
      msgtype (QString {"%1 %2 %3"}.arg(m_CQtype).arg(m_config.my_callsign()).arg(grid.left(4)),ui->tx6);
    }
    if ((m_mode=="JT4" or m_mode=="QRA64") and  ui->cbShMsgs->isChecked()) {
      if (ui->cbTx6->isChecked ()) {
        msgtype ("@1250  (SEND MSGS)", ui->tx6);
      } else {
        msgtype ("@1000  (TUNE)", ui->tx6);
      }
    }
  } else {
    ui->tx6->clear ();
  }
}

void MainWindow::genStdMsgs(QString rpt, bool unconditional)
{
  genCQMsg ();
  auto const& hisCall=ui->dxCallEntry->text();
  if(!hisCall.size ()) {
    ui->labAz->clear ();
//    ui->labDist->clear ();
    ui->tx1->clear ();
    ui->tx2->clear ();
    ui->tx3->clear ();
    ui->tx4->clear ();
    if (unconditional) {        // leave in place in case it needs
                                // sending again
      ui->tx5->lineEdit ()->clear ();
    }
    ui->genMsg->clear ();
    m_gen_message_is_cq = false;
    return;
  }
  auto const& my_callsign = m_config.my_callsign ();
  auto is_compound = my_callsign != m_baseCall;
  auto is_type_one = is_compound && shortList (my_callsign);
  auto const& my_grid = m_config.my_grid ().left (4);
  auto const& hisBase = Radio::base_callsign (hisCall);
  auto eme_short_codes = m_config.enable_VHF_features () && ui->cbShMsgs->isChecked () && m_mode == "JT65";
  QString t0=hisBase + " " + m_baseCall + " ";
  QString t00=t0;
  QString t {t0 + my_grid};
  msgtype(t, ui->tx1);
  if (eme_short_codes) {
    t=t+" OOO";
    msgtype(t, ui->tx2);
    msgtype("RO", ui->tx3);
    msgtype(m_send_RR73 ? "RR73" : "RRR", ui->tx4);
    msgtype("73", ui->tx5->lineEdit ());
  } else {
    int n=rpt.toInt();
    rpt.sprintf("%+2.2d",n);

    if(m_mode=="MSK144" or m_mode=="FT8") {
      if(ui->cbVHFcontest->isChecked()) {
        t=t0 + my_grid;
        msgtype(t, ui->tx2);
        t=t0 + "R " + my_grid;
        msgtype(t, ui->tx3);
      }
    }
    if(m_mode=="MSK144" and m_bShMsgs) {
      int i=t0.length()-1;
      t0="<" + t0.mid(0,i) + "> ";
      if(!ui->cbVHFcontest->isChecked()) {
        if(n<=-2) n=-3;
        if(n>=-1 and n<=1) n=0;
        if(n>=2 and n<=4) n=3;
        if(n>=5 and n<=7) n=6;
        if(n>=8 and n<=11) n=10;
        if(n>=12 and n<=14) n=13;
        if(n>=15) n=16;
        rpt.sprintf("%+2.2d",n);
      }
    }
    if((m_mode!="MSK144" and m_mode!="FT8") or !ui->cbVHFcontest->isChecked()) {
      t=t00 + rpt;
      msgtype(t, ui->tx2);
      t=t0 + "R" + rpt;
      msgtype(t, ui->tx3);
    }
    t=t0 + (m_send_RR73 ? "RR73" : "RRR");
    if ((m_mode=="JT4" || m_mode=="QRA64") && m_bShMsgs) t="@1500  (RRR)";
    msgtype(t, ui->tx4);
    t=t0 + "73";
    if (m_mode=="JT4" || m_mode=="QRA64") {
      if (m_bShMsgs) t="@1750  (73)";
      msgtype(t, ui->tx5->lineEdit ());
    }
    else if ("MSK144" == m_mode && m_bShMsgs) {
      msgtype(t, ui->tx5->lineEdit ());
    }
    else if (unconditional || hisBase != m_lastCallsign || !m_lastCallsign.size ()) {
      // only update tx5 when forced or  callsign changes
      msgtype(t, ui->tx5->lineEdit ());
      m_lastCallsign = hisBase;
    }
  }

  if (is_compound) {
    if (is_type_one) {
      t=hisBase + " " + my_callsign;
      msgtype(t, ui->tx1);
    } else {
      t = "DE " + my_callsign + " ";
      switch (m_config.type_2_msg_gen ())
        {
        case Configuration::type_2_msg_1_full:
          msgtype(t + my_grid, ui->tx1);
          if (!eme_short_codes) {
            if ((m_mode=="MSK144" || m_mode=="FT8")
                && ui->cbVHFcontest->isChecked()) {
              msgtype(t + "R " + my_grid, ui->tx3);
            }
            else {
              msgtype(t + "R" + rpt, ui->tx3);
            }
            if ((m_mode != "JT4" && m_mode != "QRA64") || !m_bShMsgs) {
              msgtype(t + "73", ui->tx5->lineEdit ());
            }
          }
          break;

        case Configuration::type_2_msg_3_full:
          if ((m_mode=="MSK144" || m_mode=="FT8")
              && ui->cbVHFcontest->isChecked()) {
            msgtype(t + "R " + my_grid, ui->tx3);
            msgtype(t + "RRR", ui->tx4);
          }
          else {
            msgtype(t00 + my_grid, ui->tx1);
            msgtype(t + "R" + rpt, ui->tx3);
          }
          if (!eme_short_codes && ((m_mode != "JT4" && m_mode != "QRA64") || !m_bShMsgs)) {
            msgtype(t + "73", ui->tx5->lineEdit ());
          }
          break;

        case Configuration::type_2_msg_5_only:
          msgtype(t00 + my_grid, ui->tx1);
          if (!eme_short_codes) {
            if ((m_mode=="MSK144" || m_mode=="FT8")
                && ui->cbVHFcontest->isChecked()) {
              msgtype(t + "R " + my_grid, ui->tx3);
              msgtype(t + "RRR", ui->tx4);
            }
            else {
              msgtype(t0 + "R" + rpt, ui->tx3);
            }
          }
          // don't use short codes here as in a sked with a type 2
          // prefix we would never send out prefix/suffix
          msgtype(t + "73", ui->tx5->lineEdit ());
          break;
        }
    }
    if (hisCall != hisBase
        && m_config.type_2_msg_gen () != Configuration::type_2_msg_5_only
        && !eme_short_codes) {
      // cfm we have his full call copied as we could not do this earlier
      t = hisCall + " 73";
      msgtype(t, ui->tx5->lineEdit ());
    }
  } else {
    if (hisCall != hisBase and !m_config.bHound()) {
      if (shortList(hisCall)) {
        // cfm we know his full call with a type 1 tx1 message
        t = hisCall + " " + my_callsign;
        msgtype(t, ui->tx1);
      }
      else if (!eme_short_codes
               && ("MSK144" != m_mode || !m_bShMsgs)) {
        t=hisCall + " 73";
        msgtype(t, ui->tx5->lineEdit ());
      }
    }
  }
  m_rpt=rpt;
  if(m_config.bHound() and is_compound) ui->tx1->setText("DE " + m_config.my_callsign());
}

void MainWindow::TxAgain()
{
  auto_tx_mode(true);
}

void MainWindow::clearDX ()
{
  if (m_QSOProgress != CALLING)
    {
      auto_tx_mode (false);
    }
  ui->dxCallEntry->clear ();
  ui->dxGridEntry->clear ();
  m_lastCallsign.clear ();
  m_rptSent.clear ();
  m_rptRcvd.clear ();
  m_qsoStart.clear ();
  m_qsoStop.clear ();
  genStdMsgs (QString {});
  if (ui->tabWidget->currentIndex() == 1) {
    ui->genMsg->setText(ui->tx6->text());
    m_ntx=7;
    m_gen_message_is_cq = true;
    ui->rbGenMsg->setChecked(true);
  } else {
    if(m_mode=="FT8" and m_config.bHound()) {
      m_ntx=1;
      ui->txrb1->setChecked(true);
    } else {
      m_ntx=6;
      ui->txrb6->setChecked(true);
    }
  }
  m_QSOProgress = CALLING;
}

void MainWindow::lookup()                                       //lookup()
{
  QString hisCall {ui->dxCallEntry->text()};
  if (!hisCall.size ()) return;
  QFile f {m_config.writeable_data_dir ().absoluteFilePath ("CALL3.TXT")};
  if (f.open (QIODevice::ReadOnly | QIODevice::Text))
    {
      char c[132];
      qint64 n=0;
      for(int i=0; i<999999; i++) {
        n=f.readLine(c,sizeof(c));
        if(n <= 0) {
          ui->dxGridEntry->clear ();
          break;
        }
        QString t=QString(c);
        if(t.indexOf(hisCall)==0) {
          int i1=t.indexOf(",");
          QString hisgrid=t.mid(i1+1,6);
          i1=hisgrid.indexOf(",");
          if(i1>0) {
            hisgrid=hisgrid.mid(0,4);
          } else {
            hisgrid=hisgrid.mid(0,4) + hisgrid.mid(4,2).toLower();
          }
          ui->dxGridEntry->setText(hisgrid);
          break;
        }
      }
      f.close();
    }
}

void MainWindow::on_lookupButton_clicked()                    //Lookup button
{
  lookup();
}

void MainWindow::on_addButton_clicked()                       //Add button
{
  if(!ui->dxGridEntry->text ().size ()) {
    MessageBox::warning_message (this, tr ("Add to CALL3.TXT")
                                 , tr ("Please enter a valid grid locator"));
    return;
  }
  m_call3Modified=false;
  QString hisCall=ui->dxCallEntry->text();
  QString hisgrid=ui->dxGridEntry->text();
  QString newEntry=hisCall + "," + hisgrid;

  //  int ret = MessageBox::query_message(this, tr ("Add to CALL3.TXT"),
  //       tr ("Is %1 known to be active on EME?").arg (newEntry));
  //  if(ret==MessageBox::Yes) {
  //    newEntry += ",EME,,";
  //  } else {
  newEntry += ",,,";
  //  }

  QFile f1 {m_config.writeable_data_dir ().absoluteFilePath ("CALL3.TXT")};
  if(!f1.open(QIODevice::ReadWrite | QIODevice::Text)) {
    MessageBox::warning_message (this, tr ("Add to CALL3.TXT")
                                 , tr ("Cannot open \"%1\" for read/write: %2")
                                 .arg (f1.fileName ()).arg (f1.errorString ()));
    return;
  }
  if(f1.size()==0) {
    QTextStream out(&f1);
    out << "ZZZZZZ" << endl;
    f1.close();
    f1.open(QIODevice::ReadOnly | QIODevice::Text);
  }
  QFile f2 {m_config.writeable_data_dir ().absoluteFilePath ("CALL3.TMP")};
  if(!f2.open(QIODevice::WriteOnly | QIODevice::Text)) {
    MessageBox::warning_message (this, tr ("Add to CALL3.TXT")
                                 , tr ("Cannot open \"%1\" for writing: %2")
                                 .arg (f2.fileName ()).arg (f2.errorString ()));
    return;
  }
  QTextStream in(&f1);          //Read from CALL3.TXT
  QTextStream out(&f2);         //Copy into CALL3.TMP
  QString hc=hisCall;
  QString hc1="";
  QString hc2="000000";
  QString s;
  do {
    s=in.readLine();
    hc1=hc2;
    if(s.mid(0,2)=="//") {
      out << s + QChar::LineFeed; //Copy all comment lines
    } else {
      int i1=s.indexOf(",");
      hc2=s.mid(0,i1);
      if(hc>hc1 && hc<hc2) {
        out << newEntry + QChar::LineFeed;
        out << s + QChar::LineFeed;
        m_call3Modified=true;
      } else if(hc==hc2) {
        QString t {tr ("%1\nis already in CALL3.TXT"
                       ", do you wish to replace it?").arg (s)};
        int ret = MessageBox::query_message (this, tr ("Add to CALL3.TXT"), t);
        if(ret==MessageBox::Yes) {
          out << newEntry + QChar::LineFeed;
          m_call3Modified=true;
        }
      } else {
        if(s!="") out << s + QChar::LineFeed;
      }
    }
  } while(!s.isNull());

  f1.close();
  if(hc>hc1 && !m_call3Modified) out << newEntry + QChar::LineFeed;
  if(m_call3Modified) {
    QFile f0 {m_config.writeable_data_dir ().absoluteFilePath ("CALL3.OLD")};
    if(f0.exists()) f0.remove();
    QFile f1 {m_config.writeable_data_dir ().absoluteFilePath ("CALL3.TXT")};
    f1.rename(m_config.writeable_data_dir ().absoluteFilePath ("CALL3.OLD"));
    f2.rename(m_config.writeable_data_dir ().absoluteFilePath ("CALL3.TXT"));
    f2.close();
  }
}

void MainWindow::msgtype(QString t, QLineEdit* tx)               //msgtype()
{
  char message[29];
  char msgsent[29];
  int itone0[NUM_ISCAT_SYMBOLS];  //Dummy array, data not used
  int len1=22;
  QByteArray s=t.toUpper().toLocal8Bit();
  ba2msg(s,message);
  int ichk=1,itype=0;
  gen65_(message,&ichk,msgsent,itone0,&itype,len1,len1);
  msgsent[22]=0;
  bool text=false;
  bool shortMsg=false;
  if(itype==6) text=true;
  if(itype==7 and m_config.enable_VHF_features() and
     m_mode=="JT65") shortMsg=true;
  if(m_mode=="MSK144" and t.mid(0,1)=="<") text=false;
  if((m_mode=="MSK144" or m_mode=="FT8") and ui->cbVHFcontest->isChecked()) {
    int i0=t.trimmed().length()-7;
    if(t.mid(i0,3)==" R ") text=false;
  }
  QPalette p(tx->palette());
  if(text) {
    p.setColor(QPalette::Base,"#ffccff");
  } else {
    if(shortMsg) {
      p.setColor(QPalette::Base,"#66ffff");
    } else {
      p.setColor(QPalette::Base,Qt::transparent);
      if(m_mode=="MSK144" and t.mid(0,1)=="<") {
        p.setColor(QPalette::Base,"#00ffff");
      }
    }
  }
  tx->setPalette(p);
  auto pos  = tx->cursorPosition ();
  tx->setText(t.toUpper());
  tx->setCursorPosition (pos);
}

void MainWindow::on_tx1_editingFinished()                       //tx1 edited
{
  QString t=ui->tx1->text();
  msgtype(t, ui->tx1);
}

void MainWindow::on_tx2_editingFinished()                       //tx2 edited
{
  QString t=ui->tx2->text();
  msgtype(t, ui->tx2);
}

void MainWindow::on_tx3_editingFinished()                       //tx3 edited
{
  QString t=ui->tx3->text();
  msgtype(t, ui->tx3);
}

void MainWindow::on_tx4_editingFinished()                       //tx4 edited
{
  QString t=ui->tx4->text();
  msgtype(t, ui->tx4);
}

void MainWindow::on_tx5_currentTextChanged (QString const& text) //tx5 edited
{
  msgtype(text, ui->tx5->lineEdit ());
}

void MainWindow::on_tx6_editingFinished()                       //tx6 edited
{
  QString t=ui->tx6->text().toUpper();
  if(t.indexOf(" ")>0) {
    QString t1=t.split(" ").at(1);
    m_CQtype="CQ";
    if(t1.size()==2) m_CQtype="CQ " + t1;
  }
  msgtype(t, ui->tx6);
}

void MainWindow::cacheActivity(QString key){
    m_callActivityCache[key] = m_callActivity;
    m_bandActivityCache[key] = m_bandActivity;
    m_rxTextCache[key] = ui->textEditRX->toHtml();
}

void MainWindow::restoreActivity(QString key){
    if(m_callActivityCache.contains(key)){
        m_callActivity = m_callActivityCache[key];
    }

    if(m_bandActivityCache.contains(key)){
        m_bandActivity = m_bandActivityCache[key];
    }

    if(m_rxTextCache.contains(key)){
        ui->textEditRX->setHtml(m_rxTextCache[key]);
    }

    displayActivity(true);
}

void MainWindow::clearActivity(){
    m_bandActivity.clear();
    m_callActivity.clear();
    m_callSeenBeacon.clear();
    m_compoundCallCache.clear();
    m_rxCallCache.clear();
    m_rxCallQueue.clear();
    m_rxRecentCache.clear();
    m_rxDirectedCache.clear();
    m_rxFrameBlockNumbers.clear();
    m_rxActivityQueue.clear();
    m_rxCommandQueue.clear();
    m_lastTxMessage.clear();

    clearTableWidget(ui->tableWidgetCalls);

    // this is now duplicated in three places :(
    ui->tableWidgetCalls->insertRow(ui->tableWidgetCalls->rowCount());
    auto item = new QTableWidgetItem("ALLCALL");
    item->setData(Qt::UserRole, QVariant("ALLCALL"));
    ui->tableWidgetCalls->setItem(ui->tableWidgetCalls->rowCount() - 1, 0, item);
    ui->tableWidgetCalls->setSpan(ui->tableWidgetCalls->rowCount() - 1, 0, 1, ui->tableWidgetCalls->columnCount());

    clearTableWidget(ui->tableWidgetRXAll);

    ui->textEditRX->clear();
    ui->freeTextMsg->clear();
    ui->extFreeTextMsg->clear();

    // make sure to clear the read only and transmitting flags so there's always a "way out"
    ui->extFreeTextMsgEdit->clear();
    ui->extFreeTextMsgEdit->setReadOnly(false);
    update_dynamic_property(ui->extFreeTextMsgEdit, "transmitting", false);
}

void MainWindow::displayTextForFreq(QString text, int freq, QDateTime date, bool isTx, bool isNewLine, bool isLast){
    int lowFreq = freq/10*10;
    int highFreq = lowFreq + 10;

    int block = -1;

    if(m_rxFrameBlockNumbers.contains(freq)){
        block = m_rxFrameBlockNumbers[freq];
    } else if(m_rxFrameBlockNumbers.contains(lowFreq)){
        block = m_rxFrameBlockNumbers[lowFreq];
        freq = lowFreq;
    } else if(m_rxFrameBlockNumbers.contains(highFreq)){
        block = m_rxFrameBlockNumbers[highFreq];
        freq = highFreq;
    }

    if(isNewLine){
        //m_rxFrameBlockNumbers.remove(freq);
        m_rxFrameBlockNumbers.remove(lowFreq);
        m_rxFrameBlockNumbers.remove(highFreq);
        block = -1;
    }

    block = writeMessageTextToUI(date, text, freq, isTx, block);

    // never cache tx or last lines
    if(isTx || isLast) {
        // pass
    } else {
        m_rxFrameBlockNumbers.insert(freq, block);
        m_rxFrameBlockNumbers.insert(lowFreq, block);
        m_rxFrameBlockNumbers.insert(highFreq, block);
    }
}

void MainWindow::writeNoticeTextToUI(QDateTime date, QString text){
    auto c = ui->textEditRX->textCursor();
    c.movePosition(QTextCursor::End);
    if(c.block().length() > 1){
        c.insertBlock();
    }

    text = text.toHtmlEscaped();
    c.insertBlock();
    c.insertHtml(QString("<strong>%1 - %2</strong>").arg(date.time().toString()).arg(text));

    c.movePosition(QTextCursor::End);

    ui->textEditRX->ensureCursorVisible();
    ui->textEditRX->verticalScrollBar()->setValue(ui->textEditRX->verticalScrollBar()->maximum());
}


int MainWindow::writeMessageTextToUI(QDateTime date, QString text, int freq, bool bold, int block){
    auto c = ui->textEditRX->textCursor();

    // find an existing block (
    bool found = false;
    if(block != -1){
        QTextBlock b = c.document()->findBlockByNumber(block);
        c.setPosition(b.position());
        c.movePosition(QTextCursor::EndOfBlock);
        found = true;
    } else {
        c.movePosition(QTextCursor::End);
        if(c.block().length() > 1){
            c.insertBlock();
        }
    }

    if(found && !bold){
        c.clearSelection();
        c.insertText(text);
    } else {
        text = text.toHtmlEscaped();
        if(bold){
            text = QString("<strong>%1</strong>").arg(text);
        }
        c.insertBlock();
        c.insertHtml(QString("<strong>%1 - (%2)</strong> - %3").arg(date.time().toString()).arg(freq).arg(text));
    }

    c.movePosition(QTextCursor::End);

    ui->textEditRX->ensureCursorVisible();
    ui->textEditRX->verticalScrollBar()->setValue(ui->textEditRX->verticalScrollBar()->maximum());

    return c.blockNumber();
}

bool MainWindow::isMessageQueuedForTransmit(){
    return m_transmitting || m_txFrameCount > 0;
}

void MainWindow::addMessageText(QString text, bool clear, bool selectFirstPlaceholder){
    // don't add message text if we already have a transmission queued...
    if(isMessageQueuedForTransmit()){
        return;
    }

    if(clear){
        ui->extFreeTextMsgEdit->clear();
    }

    // TODO: jsherer - check to make sure we're not transmitting currently
    QTextCursor c = ui->extFreeTextMsgEdit->textCursor();
    if(c.hasSelection()){
        c.removeSelectedText();
    }

    int pos = c.position();
    c.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);

    bool isSpace = c.selectedText().isEmpty() || c.selectedText().at(0).isSpace();
    c.clearSelection();

    c.setPosition(pos);

    if(!isSpace){
        c.insertText(" ");
    }

    c.insertText(text);

    if(selectFirstPlaceholder){
        auto match = QRegularExpression("(\\[.+\\])").match(ui->extFreeTextMsgEdit->toPlainText());
        if(match.hasMatch()){
            c.setPosition(match.capturedStart());
            c.setPosition(match.capturedEnd(), QTextCursor::KeepAnchor);
            ui->extFreeTextMsgEdit->setTextCursor(c);
        }
    }

    ui->extFreeTextMsgEdit->setFocus();
}

void MainWindow::enqueueMessage(int priority, QString message, int freq, Callback c){
    m_txMessageQueue.enqueue(
        PrioritizedMessage{
            QDateTime::currentDateTimeUtc(), priority, message, freq, c
        }
    );
}

void MainWindow::resetMessage(){
    resetMessageUI();
    resetMessageTransmitQueue();
}

void MainWindow::resetMessageUI(){
    ui->nextFreeTextMsg->clear();
    ui->extFreeTextMsg->clear();
    ui->extFreeTextMsgEdit->clear();
    ui->extFreeTextMsgEdit->setReadOnly(false);
    update_dynamic_property (ui->extFreeTextMsgEdit, "transmitting", false);

    if(ui->startTxButton->isChecked()){
        ui->startTxButton->setChecked(false);
    }
}

bool MainWindow::ensureCallsignSet(bool alert){
    if(m_config.my_callsign().trimmed().isEmpty()){
        if(alert) MessageBox::warning_message(this, tr ("Please enter your callsign in the settings."));
        on_actionSettings_triggered();
        return false;
    }

    if(m_config.my_grid().trimmed().isEmpty()){
        if(alert) MessageBox::warning_message(this, tr ("Please enter your grid locator in the settings."));
        on_actionSettings_triggered();
        return false;
    }

    return true;
}

void MainWindow::createMessage(QString const& text){
    if(!ensureCallsignSet()){
        on_stopTxButton_clicked();
        return;
    }

    resetMessageTransmitQueue();
    createMessageTransmitQueue(text);
}

void MainWindow::createMessageTransmitQueue(QString const& text){
  auto frames = buildFT8MessageFrames(text);

  m_txFrameQueue.append(frames);
  m_txFrameCount = frames.length();

  int freq = currentFreqOffset();

  QStringList lines;
  foreach(auto frame, frames){
      auto dt = DecodedText(frame);
      lines.append(dt.message());
  }

  displayTextForFreq(lines.join(""), freq, QDateTime::currentDateTimeUtc(), true, true, true);

  // keep track of the last message text sent
  m_lastTxMessage = text;
}

void MainWindow::restoreMessage(){
    if(m_lastTxMessage.isEmpty()){
        return;
    }
    addMessageText(m_lastTxMessage, true);
}

void MainWindow::resetMessageTransmitQueue(){
  m_txFrameCount = 0;
  m_txFrameQueue.clear();
  m_txMessageQueue.clear();
}

QString MainWindow::popMessageFrame(){
  if(m_txFrameQueue.isEmpty()){
      return QString();
  }
  return m_txFrameQueue.dequeue();
}

void MainWindow::on_nextFreeTextMsg_currentTextChanged (QString const& text)
{
  msgtype(text, ui->nextFreeTextMsg);
}

void MainWindow::on_extFreeTextMsgEdit_currentTextChanged (QString const& text)
{
    QString x;
    QString::const_iterator i;
    auto validChars = Varicode::huffValidChars();
    for(i = text.constBegin(); i != text.constEnd(); i++){
        auto ch = (*i).toUpper();
        if(validChars.contains(ch) || ch == '\n'){
            x += ch;
        }
    }
    if(x != text){
      int pos = ui->extFreeTextMsgEdit->textCursor().position();
      int maxpos = x.size();
      ui->extFreeTextMsgEdit->setPlainText(x);
      QTextCursor c = ui->extFreeTextMsgEdit->textCursor();
      c.setPosition(pos < maxpos ? pos : maxpos, QTextCursor::MoveAnchor);
      ui->extFreeTextMsgEdit->setTextCursor(c);
    }

    int count = countFT8MessageFrames(x);
    if(count > 0){
        ui->startTxButton->setText(QString("Send (%1)").arg(count));
        ui->startTxButton->setEnabled(true);
    } else {
        ui->startTxButton->setText("Send");
        ui->startTxButton->setEnabled(false);
    }
}

int MainWindow::currentFreqOffset(){
    return ui->RxFreqSpinBox->value();
}

int MainWindow::countFT8MessageFrames(QString const& text){
    return buildFT8MessageFrames(text).length();
}

QStringList MainWindow::buildFT8MessageFrames(QString const& text){
    #define ALLOW_SEND_COMPOUND 1
    #define AUTO_PREPEND_DIRECTED 1

    QStringList frames;

    // prepare selected callsign for directed message
    QString selectedCall = callsignSelected();

    // prepare compound
    bool compound = Radio::is_compound_callsign(m_config.my_callsign());
    QString mygrid = m_config.my_grid().left(4);
    QString mycall = m_config.my_callsign();
    QString basecall = Radio::base_callsign(m_config.my_callsign());
    if(basecall != mycall){
        basecall = "<....>";
    }

    foreach(QString line, text.split(QRegExp("[\\r\\n]"), QString::SkipEmptyParts)){
        // once we find a directed call, data encode the rest of the line.
        bool hasDirected = false;

        // do the same for when we have sent data...
        bool hasData = false;

        // remove our callsign from the start of the line...
        if(line.startsWith(mycall + ":")){
            line = lstrip(line.mid(mycall.length() + 1));
        }
        if(line.startsWith(basecall + ":")){
            line = lstrip(line.mid(basecall.length() + 1));
        }

#if AUTO_PREPEND_DIRECTED
        // see if we need to prepend the directed call to the line...
        // if we have a selected call and the text doesn't start with that call...
        // and if this isn't a raw message (starting with "<")... then...
        if(!selectedCall.isEmpty() && !line.startsWith(selectedCall) && !line.startsWith("<")){
            auto calls = Varicode::parseCallsigns(line);

            bool lineStartsWithBaseCall = (
                line.startsWith("ALLCALL") ||
                line.startsWith("CQCQCQ")  ||
                line.startsWith("BEACON")
            );

            bool lineStartsWithStandardCall = !calls.isEmpty() && line.startsWith(calls.first());

            if(lineStartsWithBaseCall || lineStartsWithStandardCall){
                // pass
            } else {
                // if the message doesn't start with a base call
                // and if there are no other callsigns in this message
                // or if the first callsign in the message isn't at the beginning...
                // then we should be auto-prefixing this line with the selected call

                line = QString("%1 %2").arg(selectedCall).arg(line);
            }
        }
#endif

        while(line.size() > 0){
          QString frame;

          bool useStd = false;
          bool useBcn = false;
#if ALLOW_SEND_COMPOUND
          bool useCmp = false;
#endif
          bool useDir = false;
          bool useDat = false;
          bool isFree = false;
          QString stdFrame = parseFT8Message(line, &isFree);

          int l = 0;
          QString bcnFrame = Varicode::packBeaconMessage(line, mycall, &l);

#if ALLOW_SEND_COMPOUND
          int o = 0;
          QString cmpFrame = Varicode::packCompoundMessage(line, &o);
#endif

          int n = 0;
          QString dirCmd;
          QString dirTo;
          QString dirNum;
          QString dirFrame = Varicode::packDirectedMessage(line, basecall, &dirTo, &dirCmd, &dirNum, &n);
          bool dirToCompound = dirTo.contains("/");

          int m = 0;
          QString datFrame = Varicode::packDataMessage(line.left(24) + "\x04", &m); //  66 / 3 + 2 = 22 (maximum number of 3bit chars we could possibly stuff in here plus 2 for good measure :P)

          // if this parses to a standard FT8 free text message
          // but it can be parsed as a directed message, then we
          // should send the directed version. if we've already sent
          // a directed message or a data frame, we will only follow it
          // with more data frames.

          if(isFree && !hasDirected && !hasData && l > 0){
              useBcn = true;
              hasDirected = false;
              frame = bcnFrame;
          }
#if ALLOW_SEND_COMPOUND
          else if(isFree && !hasDirected && !hasData && o > 0){
              useCmp = true;
              hasDirected = false;
              frame = cmpFrame;
          }
#endif
          else if(isFree && !hasDirected && !hasData && n > 0){
              useDir = true;
              hasDirected = true;
              frame = dirFrame;
          }
          else if ((isFree || hasDirected) && m > 0) {
              useDat = true;
              hasData = true;
              frame = datFrame;
          } else {
              useStd = true;
              frame = stdFrame;
          }

          if(useStd){
              if(frame.isEmpty()){
                break;
              }
              frames.append(frame);

              if(!line.startsWith(frame)){
                  line = (
                    line.left(frame.length()).replace(':', ' ') + // is this the only case where we could have a mismatch?
                    line.mid(frame.length())
                  ).trimmed();
              }

              line = line.mid(frame.length()).trimmed();
          }

          if(useBcn){
              frames.append(frame);
              line = line.mid(l);
          }

#if ALLOW_SEND_COMPOUND
          if(useCmp){
              frames.append(frame);
              line = line.mid(o);
          }
#endif

          if(useDir){
              /**
               * We have a few special cases when we are sending to a compound call, or our call is a compound call, or both.
               * CASE 0: Non-compound:       KN4CRD: J1Y ACK
               * -> One standard directed message frame
               *
               * CASE 1: Compound From:      KN4CRD/P: J1Y ACK
               * -> One standard compound frame, followed by a standard directed message frame with placeholder
               * -> The second standard directed frame _could_ be replaced with a compound directed frame
               * -> <KN4CRD/P EM73> then <....>: J1Y ACK
               * -> <KN4CRD/P EM73> then <J1Y ACK>
               *
               * CASE 2: Compound To:        KN4CRD: J1Y/P ACK
               * -> One standard compound frame, followed by a compound directed frame
               * -> <KN4CRD EM73> then <J1Y/P ACK>
               *
               * CASE 3: Compound From & To: KN4CRD/P: J1Y/P ACK
               * -> One standard compound frame, followed by a compound directed frame
               * -> <KN4CRD/P EM73> then <J1Y/P ACK>
               **/
              bool shouldUseStandardFrame = true;
              if(compound || dirToCompound){
                  // Cases 1, 2, 3 all send a standard compound frame first...
                  QString deCompoundMessage = QString("<%1 %2>").arg(mycall).arg(mygrid);
                  QString deCompoundFrame = Varicode::packCompoundMessage(deCompoundMessage, nullptr);
                  if(!deCompoundFrame.isEmpty()){
                      frames.append(deCompoundFrame);
                  }

                  // Followed, by a standard OR compound directed message...
                  QString dirCompoundMessage = QString("<%1%2%3>").arg(dirTo).arg(dirCmd).arg(dirNum);
                  QString dirCompoundFrame = Varicode::packCompoundMessage(dirCompoundMessage, nullptr);
                  if(!dirCompoundFrame.isEmpty()){
                      frames.append(dirCompoundFrame);
                  }
                  shouldUseStandardFrame = false;
              }

              if(shouldUseStandardFrame) {
                  // otherwise, just send the standard directed frame
                  frames.append(frame);
              }

              line = line.mid(n);

              // generate a checksum for buffered commands with line data
              if(Varicode::isCommandBuffered(dirCmd) && !line.isEmpty()){
                  qDebug() << "generating checksum for line" << line << line.mid(1);

                  // strip leading whitespace after a buffered directed command
                  line = lstrip(line);

                  qDebug() << "before:" << line;
                  int checksumSize = Varicode::isCommandChecksumed(dirCmd);

                  if(checksumSize == 32){
                      line = line + " " + Varicode::checksum32(line);
                  } else if (checksumSize == 16) {
                      line = line + " " + Varicode::checksum16(line);
                  } else if (checksumSize == 0) {
                      // pass
                  }
                  qDebug() << "after:" << line;
              }
          }

          if(useDat){
              frames.append(frame);
              line = line.mid(m);
          }
        }
    }

#if 1
    qDebug() << "parsed frames:";
    foreach(auto frame, frames){
        auto dt = DecodedText(frame);
        qDebug() << "->" << frame << dt.message() << Varicode::frameTypeString(dt.frameType());
    }
#endif

    return frames;
}


QString MainWindow::parseFT8Message(QString input, bool *isFree){
  if(isFree) *isFree = true;
  return input;

#if 0
  char message[29];
  char msgsent[29];
  char volatile ft8msgbits[75 + 12];
  int volatile itone[NUM_ISCAT_SYMBOLS];

  QByteArray ba = input.toLocal8Bit();
  ba2msg(ba,message);

  qint32  i3bit = 0;
  bool bcontest = false;
  char MyGrid[6];
  strncpy(MyGrid, (m_config.my_grid()+"      ").toLatin1(),6);
  genft8_(message, MyGrid, &bcontest, &i3bit, msgsent, const_cast<char *> (ft8msgbits),
        const_cast<int *> (itone), 22, 6, 22);
  msgsent[22]=0;

  // decode the msg bits into 6-bit bytes so we can check to see if its a free text message or not...
  // see extractmessage1764.f90 for the original implementation. we could technically add a boolean output
  // from the fortran code, but this avoids having to modify that so we can easily apply updates to the
  // signal functions in the future without incurring too much cognitive overhead of merge conflicts.
  char msgbytes[12];
  for(int ibyte = 1; ibyte <= 12; ibyte++){
      int itmp = 0;
      for(int ibit = 1; ibit <= 6; ibit++){
          itmp = (itmp << 1) + (1 & (ft8msgbits[((ibyte-1) * 6 + ibit)-1]));
      }
      msgbytes[ibyte-1] = itmp;
  }

  int a = msgbytes[9];
  int b = msgbytes[10];
  int c = msgbytes[11];
  int d = ((a & 15) << 12) + (b << 6) + c;

  QString output = QString::fromLatin1(msgsent);

  if(isFree){
      // TODO: jsherer - lets figure out a better way to detect this for the case:
      //    KN4CRD 16
      // this gets encoded as a standard message, but doesn't seem to make sense as to why...
      // see decodedtext.cpp for the alternate decoding side of this...
      *isFree = (d >= 32768) || !QRegularExpression("^(CQ|DE|QRZ)\\s").match(output).hasMatch();
  }

  return output.trimmed();
#endif
}

bool MainWindow::prepareNextMessageFrame()
{
  m_i3bit = Varicode::FT8Call;

  QString frame = popMessageFrame();
  if(frame.isEmpty()){  
    ui->nextFreeTextMsg->clear();
    return false;
  } else {
    ui->nextFreeTextMsg->setText(frame);

    int count = m_txFrameCount;
    int sent = count - m_txFrameQueue.count();

    if(sent == 1){
        m_i3bit |= Varicode::FT8CallFirst;
    }
    if(count == sent){
        m_i3bit |= Varicode::FT8CallLast;
    }

    ui->startTxButton->setText(QString("Sending (%1/%2)").arg(sent).arg(count));

    if(ui->beaconButton->isChecked()){
        // bump beacon
        scheduleBacon(false);
    }

    return true;
  }
}

bool MainWindow::isFreqOffsetFree(int f, int bw){
    // if this frequency is our current frequency, it's always "free"
    if(currentFreqOffset() == f){
        return true;
    }

    // if this frequency is in our directed cache, it's always "free"
    if(isDirectedOffset(f, nullptr)){
        return true;
    }

    foreach(int offset, m_bandActivity.keys()){
        auto d = m_bandActivity[offset];
        if(d.isEmpty()){
            continue;
        }

        // if we last received on this more than 30 seconds ago, it's free
        if(d.last().utcTimestamp.secsTo(QDateTime::currentDateTimeUtc()) >= 30){
            continue;
        }

        // otherwise, if this is an occupied slot within our 60Hz of where we'd like to transmit... it's not free...
        if(qAbs(offset - f) < bw){
            return false;
        }
    }

    return true;
}

int MainWindow::findFreeFreqOffset(int fmin, int fmax, int bw){
    int nslots = (fmax-fmin)/bw;

    int f = fmin;
    for(int i = 0; i < nslots; i++){
        f = fmin + bw * (qrand() % nslots);
        if(isFreqOffsetFree(f, bw)){
            return f;
        }
    }

    for(int i = 0; i < nslots; i++){
        f = fmin + (qrand() % (fmax-fmin));
        if(isFreqOffsetFree(f, bw)){
            return f;
        }
    }

    // return 0 if there's no free offset
    return 0;
}

// scheduleBeacon
void MainWindow::scheduleBacon(bool first){
    auto timestamp = QDateTime::currentDateTimeUtc();
    auto orig = timestamp;

    // remove milliseconds
    auto t = timestamp.time();
    t.setHMS(t.hour(), t.minute(), t.second());
    timestamp.setTime(t);

    // round to 15 second increment
    int secondsSinceEpoch = (timestamp.toMSecsSinceEpoch()/1000);
    int delta = roundUp(secondsSinceEpoch, 15) + 1 + (first ? /*m_txFirst ? 15 : 30*/ 0 : qMax(1, m_config.beacon()) * 60) - secondsSinceEpoch;
    timestamp = timestamp.addSecs(delta);

    // 25% of the time, switch intervals
    float prob = (float) qrand() / (RAND_MAX);
    if(prob < 0.25){
        timestamp = timestamp.addSecs(15);
    }

    m_nextBeacon = timestamp;
    m_nextBeaconQueued = false;
    m_nextBeaconPaused = false;

    if(!beaconTimer.isActive()){
        beaconTimer.setInterval(1000);
        beaconTimer.start();
    }
}

// pauseBeacon
void MainWindow::pauseBacon(){
    ui->beaconButton->setChecked(false);
    m_nextBeaconPaused = true;

    if(beaconTimer.isActive()){
        beaconTimer.stop();
    }
}

// checkBeacon
void MainWindow::checkBacon(){
    if(!ui->beaconButton->isChecked()){
        return;
    }
    if(QDateTime::currentDateTimeUtc().secsTo(m_nextBeacon) > 5){
        return;
    }
    if(m_nextBeaconQueued){
        return;
    }

    qDebug() << "PREPARING BACON!!";
    prepareBacon();
}

// prepareBeacon
void MainWindow::prepareBacon(){
    QStringList lines;

    QString mycall = m_config.my_callsign();
    QString mygrid = m_config.my_grid().left(4);

    // FT8Call Style
    lines.append(QString("%1: BEACON %2").arg(mycall).arg(mygrid));

    bool shouldTransmitTwoBeacons = true;
    if(shouldTransmitTwoBeacons){
        lines.append(QString("%1: BEACON %2").arg(mycall).arg(mygrid));
    }

    // Queue the beacon
    enqueueMessage(PriorityLow, lines.join(QChar('\n')), currentFreqOffset(), [this](){
        m_nextBeaconQueued = false;
    });

    m_nextBeaconQueued = true;
}



QString MainWindow::calculateDistance(QString const& value, int *pDistance)
{
    QString grid = value.trimmed();
    if(grid.isEmpty() || grid.length() < 4){
        return QString{};
    }

    qint64 nsec = (QDateTime::currentMSecsSinceEpoch()/1000) % 86400;
    double utch=nsec/3600.0;
    int nAz,nEl,nDmiles,nDkm,nHotAz,nHotABetter;
    azdist_(const_cast <char *> ((m_config.my_grid () + "      ").left (6).toLatin1().constData()),
            const_cast <char *> ((grid + "      ").left (6).toLatin1().constData()),&utch,
            &nAz,&nEl,&nDmiles,&nDkm,&nHotAz,&nHotABetter,6,6);

    if(m_config.miles()){
        if(pDistance) *pDistance = nDmiles;
        return QString("%1 mi").arg(nDmiles);
    }

    if(pDistance) *pDistance = nDkm;
    return QString("%1 km").arg(nDkm);
}

// this function is called by auto_tx_mode, which is called by autoButton.clicked
void MainWindow::on_startTxButton_toggled(bool checked)
{
    if(checked){       
        createMessage(ui->extFreeTextMsgEdit->toPlainText());
        startTx();
    } else {
        resetMessage();
        stopTx();
        on_stopTxButton_clicked();
    }
}

void MainWindow::toggleTx(bool start){
    if(start && ui->startTxButton->isChecked()) { return; }
    if(!start && !ui->startTxButton->isChecked()) { return; }
    ui->startTxButton->setChecked(start);
}

void MainWindow::on_rbNextFreeTextMsg_toggled (bool status)
{
  if (status) {
    m_ntx = 9;
  }
}

void MainWindow::on_dxCallEntry_textChanged (QString const& call)
{
  m_hisCall = call;
  statusChanged();
  statusUpdate ();
}

void MainWindow::on_dxCallEntry_returnPressed ()
{
  on_lookupButton_clicked();
}

void MainWindow::on_dxGridEntry_textChanged (QString const& grid)
{
  if (ui->dxGridEntry->hasAcceptableInput ()) {
    if (grid != m_hisGrid) {
      m_hisGrid = grid;
      statusUpdate ();
    }
    qint64 nsec = (QDateTime::currentMSecsSinceEpoch()/1000) % 86400;
    double utch=nsec/3600.0;
    int nAz,nEl,nDmiles,nDkm,nHotAz,nHotABetter;
    azdist_(const_cast <char *> ((m_config.my_grid () + "      ").left (6).toLatin1().constData()),
            const_cast <char *> ((m_hisGrid + "      ").left (6).toLatin1().constData()),&utch,
            &nAz,&nEl,&nDmiles,&nDkm,&nHotAz,&nHotABetter,6,6);
    QString t;
    int nd=nDkm;
    if(m_config.miles()) nd=nDmiles;
    if(m_mode=="MSK144") {
      if(nHotABetter==0)t.sprintf("Az: %d   B: %d   El: %d   %d",nAz,nHotAz,nEl,nd);
      if(nHotABetter!=0)t.sprintf("Az: %d   A: %d   El: %d   %d",nAz,nHotAz,nEl,nd);
    } else {
      t.sprintf("Az: %d        %d",nAz,nd);
    }
    if(m_config.miles()) t += " mi";
    if(!m_config.miles()) t += " km";
    ui->labAz->setText (t);
  } else {
    if (m_hisGrid.size ()) {
      m_hisGrid.clear ();
      ui->labAz->clear ();
      statusUpdate ();
    }
  }
}

void MainWindow::on_genStdMsgsPushButton_clicked()         //genStdMsgs button
{
  genStdMsgs(m_rpt);
}

void MainWindow::on_logQSOButton_clicked()                 //Log QSO button
{
  /*
  if (!m_hisCall.size ()) {
    MessageBox::warning_message (this, tr ("Warning:  DX Call field is empty."));
  }
  */
  // m_dateTimeQSOOn should really already be set but we'll ensure it gets set to something just in case
  if (!m_dateTimeQSOOn.isValid ()) {
    m_dateTimeQSOOn = QDateTime::currentDateTimeUtc();
  }
  auto dateTimeQSOOff = QDateTime::currentDateTimeUtc();
  if (dateTimeQSOOff < m_dateTimeQSOOn) dateTimeQSOOff = m_dateTimeQSOOn;
  QString call=callsignSelected();
  if(call == "ALLCALL"){
      call = "";
  }
  QString grid="";
  if(m_callActivity.contains(call)){
      grid = m_callActivity[call].grid;
  }
  m_logDlg->initLogQSO (call, grid, m_modeTx == "FT8" ? "FT8CALL" : m_modeTx, m_rptSent, m_rptRcvd,
                        m_dateTimeQSOOn, dateTimeQSOOff, m_freqNominal + ui->TxFreqSpinBox->value(),
                        m_config.my_callsign(), m_config.my_grid(),
                        m_config.log_as_DATA(), m_config.report_in_comments(),
                        m_config.bFox(), m_opCall);
}

void MainWindow::acceptQSO (QDateTime const& QSO_date_off, QString const& call, QString const& grid
                            , Frequency dial_freq, QString const& mode
                            , QString const& rpt_sent, QString const& rpt_received
                            , QString const& tx_power, QString const& comments
                            , QString const& name, QDateTime const& QSO_date_on, QString const& operator_call
                            , QString const& my_call, QString const& my_grid, QByteArray const& ADIF)
{
  QString date = QSO_date_on.toString("yyyyMMdd");
  m_logBook.addAsWorked (m_hisCall, m_config.bands ()->find (m_freqNominal), m_modeTx, date);

#if 0
  m_messageClient->qso_logged (QSO_date_off, call, grid, dial_freq, mode, rpt_sent, rpt_received, tx_power, comments, name, QSO_date_on, operator_call, my_call, my_grid);
  m_messageClient->logged_ADIF (ADIF);
#endif

  sendNetworkMessage("LOG.QSO", QString(ADIF), {
      {"UTC.ON", QVariant(QSO_date_on.toMSecsSinceEpoch())},
      {"UTC.OFF", QVariant(QSO_date_off.toMSecsSinceEpoch())},
      {"CALL", QVariant(call)},
      {"GRID", QVariant(grid)},
      {"FREQ", QVariant(dial_freq)},
      {"MODE", QVariant(mode)},
      {"RPT.SENT", QVariant(rpt_sent)},
      {"RPT.RECV", QVariant(rpt_received)},
      {"NAME", QVariant(name)},
      {"COMMENTS", QVariant(comments)},
      {"STATION.OP", QVariant(operator_call)},
      {"STATION.CALL", QVariant(my_call)},
      {"STATION.GRID", QVariant(my_grid)}
  });

  if (m_config.clear_DX () and !m_config.bHound()) clearDX ();
  m_dateTimeQSOOn = QDateTime {};
}

qint64 MainWindow::nWidgets(QString t)
{
  Q_ASSERT(t.length()==N_WIDGETS);
  qint64 n=0;
  for(int i=0; i<N_WIDGETS; i++) {
    n=n + n + t.mid(i,1).toInt();
  }
  return n;
}

void MainWindow::displayWidgets(qint64 n)
{
  /* See text file "displayWidgets.txt" for widget numbers */
  qint64 j=qint64(1)<<(N_WIDGETS-1);
  bool b;
  for(int i=0; i<N_WIDGETS; i++) {
    b=(n&j) != 0;
    if(i==0) ui->txFirstCheckBox->setVisible(b);
    if(i==1) ui->TxFreqSpinBox->setVisible(b);
    if(i==2) ui->RxFreqSpinBox->setVisible(b);
    if(i==3) ui->sbFtol->setVisible(b);
    if(i==4) ui->rptSpinBox->setVisible(b);
    if(i==5) ui->sbTR->setVisible(b);
    if(i==6) {
      ui->sbCQTxFreq->setVisible (b);
      ui->cbCQTx->setVisible (b);
      auto is_compound = m_config.my_callsign () != m_baseCall;
      ui->cbCQTx->setEnabled (b && (!is_compound || shortList (m_config.my_callsign ())));
    }
    if(i==7) ui->cbShMsgs->setVisible(b);
    if(i==8) ui->cbFast9->setVisible(b);
    if(i==9) ui->cbAutoSeq->setVisible(b);
    if(i==10) ui->cbTx6->setVisible(b);
    if(i==11) ui->pbTxMode->setVisible(b);
    if(i==12) ui->pbR2T->setVisible(b);
    if(i==13) ui->pbT2R->setVisible(b);
    if(i==14) ui->cbHoldTxFreq->setVisible(b);
    if(i==14 and (!b)) ui->cbHoldTxFreq->setChecked(false);
    if(i==15) ui->sbSubmode->setVisible(b);
    if(i==16) ui->syncSpinBox->setVisible(b);
    if(i==17) ui->WSPR_controls_widget->setVisible(b);
    //if(i==18) ui->ClrAvgButton->setVisible(b);
    if(i==19) ui->actionQuickDecode->setEnabled(b);
    if(i==19) ui->actionMediumDecode->setEnabled(b);
    if(i==19) ui->actionDeepestDecode->setEnabled(b);
    if(i==20) ui->actionInclude_averaging->setVisible (b);
    if(i==21) ui->actionInclude_correlation->setVisible (b);
    if(i==22) {
      if(!b && m_echoGraph->isVisible())  m_echoGraph->hide();
    }
    if(i==23) ui->cbSWL->setVisible(b);
    if(i==24) ui->actionEnable_AP_FT8->setVisible (b);
    if(i==25) ui->actionEnable_AP_JT65->setVisible (b);
    if(i==26) ui->actionEnable_AP_DXcall->setVisible (b);
    if(i==27) ui->cbFirst->setVisible(b);
    if(i==28) ui->cbVHFcontest->setVisible(b);
    if(i==29) ui->measure_check_box->setVisible(b);
    if(i==30) ui->labDXped->setVisible(b);
    if(i==31) ui->cbRxAll->setVisible(b);
    //if(i==32) ui->cbCQonly->setVisible(b);
    j=j>>1;
  }
  ui->tabWidget->setTabEnabled(3, "FT8" == m_mode);
  m_lastCallsign.clear ();     // ensures Tx5 is updated for new modes
  genStdMsgs (m_rpt, true);
}

void MainWindow::on_actionFT8_triggered()
{
  m_mode="FT8";
  bool bVHF=m_config.enable_VHF_features();
  m_bFast9=false;
  m_bFastMode=false;
  WSPR_config(false);
  switch_mode (Modes::FT8CALL);
  m_modeTx="FT8";
  m_nsps=6912;
  m_FFTSize = m_nsps / 2;
  Q_EMIT FFTSize (m_FFTSize);
  m_hsymStop=50;
  setup_status_bar (bVHF);
  m_toneSpacing=0.0;                   //???
  ui->actionFT8->setChecked(true);     //???
  m_wideGraph->setMode(m_mode);
  m_wideGraph->setModeTx(m_modeTx);
  VHF_features_enabled(bVHF);
  ui->cbAutoSeq->setChecked(true);
  m_TRperiod=15;
  m_fastGraph->hide();
  m_wideGraph->show();
  ui->decodedTextLabel2->setText("  UTC   dB   DT Freq    Message");
  m_wideGraph->setPeriod(m_TRperiod,m_nsps);
  m_modulator->setTRPeriod(m_TRperiod); // TODO - not thread safe
  m_detector->setTRPeriod(m_TRperiod);  // TODO - not thread safe
  ui->label_7->setText("Rx Frequency");
  if(m_config.bFox()) {
    ui->label_6->setText("Stations calling DXpedition " + m_config.my_callsign());
    ui->decodedTextLabel->setText( "Call         Grid   dB  Freq   Dist Age Continent");
  } else {
    ui->label_6->setText("Band Activity");
    ui->decodedTextLabel->setText( "  UTC   dB   DT Freq    Message");
  }
  if(!bVHF) {
    displayWidgets(nWidgets("111010000100111000010000100100001"));
// Make sure that VHF contest mode is unchecked if VHF features is not enabled.
    ui->cbVHFcontest->setChecked(false);
  } else {
    displayWidgets(nWidgets("111010000100111000010000100110001"));
  }
  ui->txrb2->setEnabled(true);
  ui->txrb4->setEnabled(true);
  ui->txrb5->setEnabled(true);
  ui->txrb6->setEnabled(true);
  ui->txb2->setEnabled(true);
  ui->txb4->setEnabled(true);
  ui->txb5->setEnabled(true);
  ui->txb6->setEnabled(true);
  ui->txFirstCheckBox->setEnabled(true);
  ui->cbAutoSeq->setEnabled(true);
  if(m_config.bFox()) {
    ui->txFirstCheckBox->setChecked(true);
    ui->txFirstCheckBox->setEnabled(false);
    ui->cbHoldTxFreq->setChecked(true);
    ui->cbAutoSeq->setEnabled(false);
    ui->tabWidget->setCurrentIndex(2);
    ui->TxFreqSpinBox->setValue(300);
    displayWidgets(nWidgets("111010000100111000010000000000100"));
    ui->labDXped->setText("DXpedition: Fox");
    on_actionFox_Log_triggered();
  }
  if(m_config.bHound()) {
    ui->txFirstCheckBox->setChecked(false);
    ui->txFirstCheckBox->setEnabled(false);
    ui->cbAutoSeq->setEnabled(false);
    ui->tabWidget->setCurrentIndex(0);
    ui->cbHoldTxFreq->setChecked(true);
    displayWidgets(nWidgets("111010000100110000010000000000110"));
    ui->labDXped->setText("DXpedition: Hound");
    ui->txrb1->setChecked(true);
    ui->txrb2->setEnabled(false);
    ui->txrb4->setEnabled(false);
    ui->txrb5->setEnabled(false);
    ui->txrb6->setEnabled(false);
    ui->txb2->setEnabled(false);
    ui->txb4->setEnabled(false);
    ui->txb5->setEnabled(false);
    ui->txb6->setEnabled(false);
  }

  if((m_config.bFox() or m_config.bHound()) and !m_config.split_mode() and !m_bWarnedSplit) {
    QString errorMsg;
    MessageBox::critical_message (this,
       "Operation in FT8 DXpedition mode normally requires\n"
       " *Split* rig control (either *Rig* or *Fake It* on\n"
       "the *Settings | Radio* tab.)", errorMsg);
    m_bWarnedSplit=true;
  }
  statusChanged();
}

void MainWindow::switch_mode (Mode mode)
{
  m_fastGraph->setMode(m_mode);
  m_config.frequencies ()->filter (m_config.region (), mode);

#if 0
  auto const& row = m_config.frequencies ()->best_working_frequency (m_freqNominal);
  if (row >= 0) {
    ui->bandComboBox->setCurrentIndex (row);
    on_bandComboBox_activated (row);
  }
#endif

  ui->rptSpinBox->setSingleStep(1);
  ui->rptSpinBox->setMinimum(-50);
  ui->rptSpinBox->setMaximum(49);
  ui->sbFtol->values ({10, 20, 50, 100, 200, 500, 1000});
  if(m_mode=="MSK144") {
    ui->RxFreqSpinBox->setMinimum(1400);
    ui->RxFreqSpinBox->setMaximum(1600);
    ui->RxFreqSpinBox->setSingleStep(25);
  } else {
    ui->RxFreqSpinBox->setMinimum(0);
    ui->RxFreqSpinBox->setMaximum(5000);
    ui->RxFreqSpinBox->setSingleStep(1);
  }
  m_bVHFwarned=false;
  bool b=m_mode=="FreqCal";
  ui->tabWidget->setVisible(!b);
  if(b) {
    ui->DX_controls_widget->setVisible(false);
    ui->decodedTextBrowser2->setVisible(false);
    ui->decodedTextLabel2->setVisible(false);
    ui->label_6->setVisible(false);
    ui->label_7->setVisible(false);
  }
}

void MainWindow::WSPR_config(bool b)
{
  ui->decodedTextBrowser2->setVisible(!b);
  ui->decodedTextLabel2->setVisible(!b and ui->cbMenus->isChecked());
  ui->controls_stack_widget->setCurrentIndex (b && m_mode != "Echo" ? 1 : 0);
  //ui->QSO_controls_widget->setVisible (!b);
  //ui->DX_controls_widget->setVisible (!b);
  ui->WSPR_controls_widget->setVisible (b);
  ui->label_6->setVisible(!b and ui->cbMenus->isChecked());
  ui->label_7->setVisible(!b and ui->cbMenus->isChecked());
  //ui->logQSOButton->setVisible(!b);
  ui->DecodeButton->setEnabled(!b);
  if(b and (m_mode!="Echo")) {
    QString t="UTC    dB   DT     Freq     Drift  Call          Grid    dBm    ";
    if(m_config.miles()) t += " mi";
    if(!m_config.miles()) t += " km";
    ui->decodedTextLabel->setText(t);
    if (m_config.is_transceiver_online ()) {
      Q_EMIT m_config.transceiver_tx_frequency (0); // turn off split
    }
    m_bSimplex = true;
  } else {
    ui->decodedTextLabel->setText("UTC   dB   DT Freq    Message");
    m_bSimplex = false;
  }
  enable_DXCC_entity (m_config.DXCC ());  // sets text window proportions and (re)inits the logbook
}

void MainWindow::fast_config(bool b)
{
  m_bFastMode=b;
  ui->TxFreqSpinBox->setEnabled(!b);
  ui->sbTR->setVisible(b);
  if(b and (m_bFast9 or m_mode=="MSK144" or m_mode=="ISCAT")) {
    m_wideGraph->hide();
    m_fastGraph->show();
  } else {
    m_wideGraph->show();
    m_fastGraph->hide();
  }
}

void MainWindow::on_TxFreqSpinBox_valueChanged(int n)
{
  m_wideGraph->setTxFreq(n);
//  if (ui->cbHoldTxFreq->isChecked ()) ui->RxFreqSpinBox->setValue(n);
  if(m_mode!="MSK144") {
    Q_EMIT transmitFrequency (n - m_XIT);
  }
  statusUpdate ();
}

void MainWindow::on_RxFreqSpinBox_valueChanged(int n)
{
  m_wideGraph->setRxFreq(n);
  if (m_mode == "FreqCal") {
    setRig ();
  }
  statusUpdate ();
}

void MainWindow::on_actionQuickDecode_toggled (bool checked)
{
  m_ndepth ^= (-checked ^ m_ndepth) & 0x00000001;
}

void MainWindow::on_actionMediumDecode_toggled (bool checked)
{
  m_ndepth ^= (-checked ^ m_ndepth) & 0x00000002;
}

void MainWindow::on_actionDeepestDecode_toggled (bool checked)
{
  m_ndepth ^= (-checked ^ m_ndepth) & 0x00000003;
}

void MainWindow::on_actionInclude_averaging_toggled (bool checked)
{
  m_ndepth ^= (-checked ^ m_ndepth) & 0x00000010;
}

void MainWindow::on_actionInclude_correlation_toggled (bool checked)
{
  m_ndepth ^= (-checked ^ m_ndepth) & 0x00000020;
}

void MainWindow::on_actionEnable_AP_DXcall_toggled (bool checked)
{
  m_ndepth ^= (-checked ^ m_ndepth) & 0x00000040;
}

void MainWindow::on_actionErase_ALL_TXT_triggered()          //Erase ALL.TXT
{
  int ret = MessageBox::query_message (this, tr ("Confirm Erase"),
                                         tr ("Are you sure you want to erase file ALL.TXT?"));
  if(ret==MessageBox::Yes) {
    QFile f {m_config.writeable_data_dir ().absoluteFilePath ("ALL.TXT")};
    f.remove();
    m_RxLog=1;
  }
}

void MainWindow::on_actionErase_FoxQSO_txt_triggered()
{
  int ret = MessageBox::query_message(this, tr("Confirm Erase"),
                  tr("Are you sure you want to erase file FoxQSO.txt?"));
  if(ret==MessageBox::Yes) {
    QFile f{m_config.writeable_data_dir().absoluteFilePath("FoxQSO.txt")};
    f.remove();
  }
}

void MainWindow::on_actionErase_ft8call_log_adi_triggered()
{
  int ret = MessageBox::query_message (this, tr ("Confirm Erase"),
                                       tr ("Are you sure you want to erase file ft8call_log.adi?"));
  if(ret==MessageBox::Yes) {
    QFile f {m_config.writeable_data_dir ().absoluteFilePath ("ft8call_log.adi")};
    f.remove();
  }
}

void MainWindow::on_actionOpen_log_directory_triggered ()
{
  QDesktopServices::openUrl (QUrl::fromLocalFile (m_config.writeable_data_dir ().absolutePath ()));
}

void MainWindow::on_bandComboBox_currentIndexChanged (int index)
{
  auto const& frequencies = m_config.frequencies ();
  auto const& source_index = frequencies->mapToSource (frequencies->index (index, FrequencyList_v2::frequency_column));
  Frequency frequency {m_freqNominal};
  if (source_index.isValid ())
    {
      frequency = frequencies->frequency_list ()[source_index.row ()].frequency_;
    }

  // Lookup band
  auto const& band  = m_config.bands ()->find (frequency);
  if (!band.isEmpty ())
    {
      ui->bandComboBox->lineEdit ()->setStyleSheet ({});
      ui->bandComboBox->setCurrentText (band);
    }
  else
    {
      ui->bandComboBox->lineEdit ()->setStyleSheet ("QLineEdit {color: yellow; background-color : red;}");
      ui->bandComboBox->setCurrentText (m_config.bands ()->oob ());
    }
  displayDialFrequency ();
}

void MainWindow::on_bandComboBox_activated (int index)
{
  auto const& frequencies = m_config.frequencies ();
  auto const& source_index = frequencies->mapToSource (frequencies->index (index, FrequencyList_v2::frequency_column));
  Frequency frequency {m_freqNominal};
  if (source_index.isValid ())
    {
      frequency = frequencies->frequency_list ()[source_index.row ()].frequency_;
    }
  m_bandEdited = true;
  band_changed (frequency);
  m_wideGraph->setRxBand (m_config.bands ()->find (frequency));
}

void MainWindow::band_changed (Frequency f)
{
//  bool monitor_off=!m_monitoring;
  // Set the attenuation value if options are checked
  QString curBand = ui->bandComboBox->currentText();
  if (m_config.pwrBandTxMemory() && !m_tune) {
      if (m_pwrBandTxMemory.contains(curBand)) {
        ui->outAttenuation->setValue(m_pwrBandTxMemory[curBand].toInt());
      }
      else {
        m_pwrBandTxMemory[curBand] = ui->outAttenuation->value();
      }
  }

  if (m_bandEdited) {
    if (!m_mode.startsWith ("WSPR")) { // band hopping preserves auto Tx
      if (f + m_wideGraph->nStartFreq () > m_freqNominal + ui->TxFreqSpinBox->value ()
          || f + m_wideGraph->nStartFreq () + m_wideGraph->fSpan () <=
          m_freqNominal + ui->TxFreqSpinBox->value ()) {
//        qDebug () << "start f:" << m_wideGraph->nStartFreq () << "span:" << m_wideGraph->fSpan () << "DF:" << ui->TxFreqSpinBox->value ();
        // disable auto Tx if "blind" QSY outside of waterfall
        ui->stopTxButton->click (); // halt any transmission
        auto_tx_mode (false);       // disable auto Tx
        m_send_RR73 = false;        // force user to reassess on new band
      }
    }

    // TODO: jsherer - is this relied upon anywhere?
    //m_lastBand.clear ();
    m_bandEdited = false;

    psk_Reporter->sendReport();      // Upload any queued spots before changing band
    m_aprsClient->processQueue(true);
    if (!m_transmitting) monitor (true);
    if ("FreqCal" == m_mode)
      {
        m_frequency_list_fcal_iter = m_config.frequencies ()->find (f);
      }
    float r=m_freqNominal/(f+0.0001);
    if(r<0.9 or r>1.1) m_bVHFwarned=false;
    setRig (f);
    setXIT (ui->TxFreqSpinBox->value ());
//    if(monitor_off) monitor(false);
  }
}

void MainWindow::vhfWarning()
{
  MessageBox::warning_message (this, tr ("VHF features warning"),
     "VHF/UHF/Microwave features is enabled on a lower frequency band.");
  m_bVHFwarned=true;
}

void MainWindow::enable_DXCC_entity (bool on)
{
  if (on and !m_mode.startsWith ("WSPR") and m_mode!="Echo") {
    m_logBook.init();                        // re-read the log and cty.dat files
//    ui->gridLayout->setColumnStretch(0,55);  // adjust proportions of text displays
//    ui->gridLayout->setColumnStretch(1,45);
  } else {
//    ui->gridLayout->setColumnStretch(0,0);
//    ui->gridLayout->setColumnStretch(1,0);
  }
  updateGeometry ();
}

void MainWindow::on_pbCallCQ_clicked()
{
  genStdMsgs(m_rpt);
  ui->genMsg->setText(ui->tx6->text());
  m_ntx=7;
  m_QSOProgress = CALLING;
  m_gen_message_is_cq = true;
  ui->rbGenMsg->setChecked(true);
  if(m_transmitting) m_restart=true;
  set_dateTimeQSO(-1);
}

void MainWindow::on_pbAnswerCaller_clicked()
{
  genStdMsgs(m_rpt);
  QString t=ui->tx3->text();
  int i0=t.indexOf(" R-");
  if(i0<0) i0=t.indexOf(" R+");
  t=t.mid(0,i0+1)+t.mid(i0+2,3);
  ui->genMsg->setText(t);
  m_ntx=7;
  m_QSOProgress = REPORT;
  m_gen_message_is_cq = false;
  ui->rbGenMsg->setChecked(true);
  if(m_transmitting) m_restart=true;
  set_dateTimeQSO(2);
}

void MainWindow::on_pbSendRRR_clicked()
{
  genStdMsgs(m_rpt);
  ui->genMsg->setText(ui->tx4->text());
  m_ntx=7;
  m_QSOProgress = ROGERS;
  m_gen_message_is_cq = false;
  ui->rbGenMsg->setChecked(true);
  if(m_transmitting) m_restart=true;
}

void MainWindow::on_pbAnswerCQ_clicked()
{
  genStdMsgs(m_rpt);
  ui->genMsg->setText(ui->tx1->text());
  QString t=ui->tx2->text();
  int i0=t.indexOf("/");
  int i1=t.indexOf(" ");
  if(i0>0 and i0<i1) ui->genMsg->setText(t);
  m_ntx=7;
  m_QSOProgress = REPLYING;
  m_gen_message_is_cq = false;
  ui->rbGenMsg->setChecked(true);
  if(m_transmitting) m_restart=true;
}

void MainWindow::on_pbSendReport_clicked()
{
  genStdMsgs(m_rpt);
  ui->genMsg->setText(ui->tx3->text());
  m_ntx=7;
  m_QSOProgress = ROGER_REPORT;
  m_gen_message_is_cq = false;
  ui->rbGenMsg->setChecked(true);
  if(m_transmitting) m_restart=true;
  set_dateTimeQSO(3);
}

void MainWindow::on_pbSend73_clicked()
{
  genStdMsgs(m_rpt);
  ui->genMsg->setText(ui->tx5->currentText());
  m_ntx=7;
  m_QSOProgress = SIGNOFF;
  m_gen_message_is_cq = false;
  ui->rbGenMsg->setChecked(true);
  if(m_transmitting) m_restart=true;
}

void MainWindow::on_rbGenMsg_clicked(bool checked)
{
  m_freeText=!checked;
  if(!m_freeText) {
    if(m_ntx != 7 && m_transmitting) m_restart=true;
    m_ntx=7;
    // would like to set m_QSOProgress but what to? So leave alone and
    // assume it is correct
  }
}

void MainWindow::on_rbFreeText_clicked(bool checked)
{
  m_freeText=checked;
  if(m_freeText) {
    m_ntx=8;
    // would like to set m_QSOProgress but what to? So leave alone and
    // assume it is correct. Perhaps should store old value to be
    // restored above in on_rbGenMsg_clicked
    if (m_transmitting) m_restart=true;
  }
}

void MainWindow::on_clearAction_triggered(QObject * sender){
    // TODO: jsherer - abstract this into a tableWidgetRXAllReset function
    if(sender == ui->tableWidgetRXAll){
        m_bandActivity.clear();
        clearTableWidget(ui->tableWidgetRXAll);
    }

    // TODO: jsherer - abstract this into a tableWidgetCallsReset function
    if(sender == ui->tableWidgetCalls){
        m_callActivity.clear();
        clearTableWidget((ui->tableWidgetCalls));

        auto item = new QTableWidgetItem("ALLCALL");
        item->setData(Qt::UserRole, QVariant("ALLCALL"));
        ui->tableWidgetCalls->insertRow(ui->tableWidgetCalls->rowCount());
        ui->tableWidgetCalls->setItem(ui->tableWidgetCalls->rowCount() - 1, 0, item);
        ui->tableWidgetCalls->setSpan(ui->tableWidgetCalls->rowCount() - 1, 0, 1, ui->tableWidgetCalls->columnCount());
    }

    if(sender == ui->extFreeTextMsgEdit){
        resetMessage();
        m_lastTxMessage.clear();
    }

    if(sender == ui->textEditRX){
        // TODO: jsherer - move these
        ui->textEditRX->clear();
        m_rxFrameBlockNumbers.clear();
        m_rxActivityQueue.clear();
    }
}

void MainWindow::on_cqMacroButton_clicked(){
    QString mygrid = m_config.my_grid().left(4);
    QString text = QString("CQCQCQ %1").arg(mygrid).trimmed();
    addMessageText(text);
}

void MainWindow::on_replyMacroButton_clicked(){
    QString call = callsignSelected();
    if(call.isEmpty()){
        return;
    }
    addMessageText(QString("%1 %2").arg(call).arg(m_config.reply()));
}

void MainWindow::on_qthMacroButton_clicked(){
    QString qth = m_config.my_qth();
    if(qth.isEmpty()){
        return;
    }
    addMessageText(QString("QTH %1").arg(qth));
}

void MainWindow::setSortBy(QString key, QString value){
    m_sortCache[key] = QVariant(value);
    displayBandActivity();
    displayCallActivity();
}

QString MainWindow::getSortBy(QString key, QString defaultValue){
    return m_sortCache.value(key, QVariant(defaultValue)).toString();
}

void MainWindow::buildSortByMenu(QMenu * menu, QString key, QString defaultValue, QList<QPair<QString, QString>> values){
    auto currentSortBy = getSortBy(key, defaultValue);

    QActionGroup * g = new QActionGroup(menu);
    g->setExclusive(true);

    foreach(auto p, values){
        auto k = p.first;
        auto v = p.second;
        auto a = menu->addAction(k);
        a->setCheckable(true);
        a->setChecked(v == currentSortBy);
        a->setActionGroup(g);

        connect(a, &QAction::triggered, this, [this, a, key, v](){
            if(a->isChecked()){
                setSortBy(key, v);
            }
        });
    }
}

void MainWindow::buildBandActivitySortByMenu(QMenu * menu){
    buildSortByMenu(menu, "bandActivity", "offset", {
        {"Frequency Offset", "offset"},
        {"Last heard timestamp (oldest first)", "timestamp"},
        {"Last heard timestamp (recent first)", "-timestamp"},
        {"SNR (weakest first)", "snr"},
        {"SNR (strongest first)", "-snr"}
    });
}

void MainWindow::buildCallActivitySortByMenu(QMenu * menu){
    buildSortByMenu(menu, "callActivity", "callsign", {
        {"Callsign", "callsign"},
        {"Frequency Offset", "offset"},
        {"Distance (closest first)", "distance"},
        {"Distance (farthest first)", "-distance"},
        {"Last heard timestamp (oldest first)", "timestamp"},
        {"Last heard timestamp (recent first)", "-timestamp"},
        {"SNR (weakest first)", "snr"},
        {"SNR (strongest first)", "-snr"}
    });
}

void MainWindow::buildQueryMenu(QMenu * menu, QString call){
    bool isAllCall = isAllCallIncluded(call);

    // for now, we're going to omit displaying the call...delete this if we want the other functionality
    call = "";

    auto callAction = menu->addAction(QString("Send a directed message to selected callsign"));
    connect(callAction, &QAction::triggered, this, [this](){

        QString selectedCall = callsignSelected();
        if(selectedCall.isEmpty()){
            return;
        }

        addMessageText(QString("%1 ").arg(selectedCall), true);
    });

    menu->addSeparator();

    auto sendReplyAction = menu->addAction(QString("%1 Reply - Send reply message to selected callsign").arg(call).trimmed());
    connect(sendReplyAction, &QAction::triggered, this, [this](){
        QString selectedCall = callsignSelected();
        if(selectedCall.isEmpty()){
            return;
        }

        addMessageText(QString("%1 %2").arg(selectedCall).arg(m_config.reply()), true);
    });

    auto sendSNRAction = menu->addAction(QString("%1 SNR - Send a signal report to the selected callsign").arg(call).trimmed());
    sendSNRAction->setEnabled(m_callActivity.contains(callsignSelected()));
    connect(sendSNRAction, &QAction::triggered, this, [this](){

        QString selectedCall = callsignSelected();
        if(selectedCall.isEmpty()){
            return;
        }

        if(!m_callActivity.contains(selectedCall)){
            return;
        }

        auto d = m_callActivity[selectedCall];
        addMessageText(QString("%1 SNR %2").arg(selectedCall).arg(Varicode::formatSNR(d.snr)), true);

        if(m_config.transmit_directed()) toggleTx(true);
    });

    menu->addSeparator();

    auto snrQueryAction = menu->addAction(QString("%1? - What is my signal report?").arg(call));
    connect(snrQueryAction, &QAction::triggered, this, [this](){

        QString selectedCall = callsignSelected();
        if(selectedCall.isEmpty()){
            return;
        }

        addMessageText(QString("%1?").arg(selectedCall), true);

        if(m_config.transmit_directed()) toggleTx(true);
    });

    auto qthQueryAction = menu->addAction(QString("%1@ - What is your QTH message?").arg(call));
    qthQueryAction->setDisabled(isAllCall);
    connect(qthQueryAction, &QAction::triggered, this, [this](){

        QString selectedCall = callsignSelected();
        if(selectedCall.isEmpty()){
            return;
        }

        addMessageText(QString("%1@").arg(selectedCall), true);

        if(m_config.transmit_directed()) toggleTx(true);
    });

    auto gridQueryAction = menu->addAction(QString("%1^ - What is your current grid locator?").arg(call));
    gridQueryAction->setDisabled(isAllCall);
    connect(gridQueryAction, &QAction::triggered, this, [this](){

        QString selectedCall = callsignSelected();
        if(selectedCall.isEmpty()){
            return;
        }

        addMessageText(QString("%1^").arg(selectedCall), true);

        if(m_config.transmit_directed()) toggleTx(true);
    });

    auto stationMessageQueryAction = menu->addAction(QString("%1&& - What is your station message?").arg(call).trimmed());
    stationMessageQueryAction->setDisabled(isAllCall);
    connect(stationMessageQueryAction, &QAction::triggered, this, [this](){

        QString selectedCall = callsignSelected();
        if(selectedCall.isEmpty()){
            return;
        }

        addMessageText(QString("%1&").arg(selectedCall), true);

        if(m_config.transmit_directed()) toggleTx(true);
    });

    auto stationPowerQueryAction = menu->addAction(QString("%1% - What is your station power?").arg(call).trimmed());
    stationPowerQueryAction->setDisabled(isAllCall);
    connect(stationPowerQueryAction, &QAction::triggered, this, [this](){

        QString selectedCall = callsignSelected();
        if(selectedCall.isEmpty()){
            return;
        }

        addMessageText(QString("%1%").arg(selectedCall), true);

        if(m_config.transmit_directed()) toggleTx(true);
    });

    auto heardQueryAction = menu->addAction(QString("%1$ - What are the stations are you hearing? (Top 4 ranked by strongest SNR)").arg(call).trimmed());
    heardQueryAction->setDisabled(isAllCall);
    connect(heardQueryAction, &QAction::triggered, this, [this](){

        QString selectedCall = callsignSelected();
        if(selectedCall.isEmpty()){
            return;
        }

        addMessageText(QString("%1$").arg(selectedCall), true);

        if(m_config.transmit_directed()) toggleTx(true);
    });

    auto qsoQueryAction = menu->addAction(QString("%1 QSO [CALLSIGN]? - Can you communicate directly with [CALLSIGN]?").arg(call).trimmed());
    connect(qsoQueryAction, &QAction::triggered, this, [this](){

        QString selectedCall = callsignSelected();
        if(selectedCall.isEmpty()){
            return;
        }

        addMessageText(QString("%1 QSO [CALLSIGN]?").arg(selectedCall), true, true);
    });

    auto hashAction = menu->addAction(QString("%1#[MESSAGE] - Please ACK if you receive this message in its entirety").arg(call).trimmed());
    hashAction->setDisabled(isAllCall);
    connect(hashAction, &QAction::triggered, this, [this](){

        QString selectedCall = callsignSelected();
        if(selectedCall.isEmpty()){
            return;
        }

        addMessageText(QString("%1#[MESSAGE]").arg(selectedCall), true, true);
    });

    auto retransmitAction = menu->addAction(QString("%1|[MESSAGE] - Please ACK and retransmit the following message").arg(call).trimmed());
    retransmitAction->setDisabled(isAllCall);
    connect(retransmitAction, &QAction::triggered, this, [this](){

        QString selectedCall = callsignSelected();
        if(selectedCall.isEmpty()){
            return;
        }

        addMessageText(QString("%1|[MESSAGE]").arg(selectedCall), true, true);
    });

    auto alertAction = menu->addAction(QString("%1![MESSAGE] - Please display this message in an alert dialog and ACK if acknowledged").arg(call).trimmed());
    alertAction->setDisabled(isAllCall);
    connect(alertAction, &QAction::triggered, this, [this](){

        QString selectedCall = callsignSelected();
        if(selectedCall.isEmpty()){
            return;
        }

        addMessageText(QString("%1![MESSAGE]").arg(selectedCall), true, true);
    });

    menu->addSeparator();

    bool emptyQTC = m_config.my_station().isEmpty();
    bool emptyQTH = m_config.my_qth().isEmpty() && m_config.my_grid().isEmpty();

    auto qtcAction = menu->addAction(QString("%1 QTC - Send my station message").arg(call).trimmed());
    qtcAction->setDisabled(emptyQTC);
    connect(qtcAction, &QAction::triggered, this, [this](){

        QString selectedCall = callsignSelected();
        if(selectedCall.isEmpty()){
            return;
        }

        addMessageText(QString("%1 QTC %2").arg(selectedCall).arg(m_config.my_station()), true);

        if(m_config.transmit_directed()) toggleTx(true);
    });

    auto qthAction = menu->addAction(QString("%1 QTH - Send my station location message").arg(call).trimmed());
    qthAction->setDisabled(emptyQTH);
    connect(qthAction, &QAction::triggered, this, [this](){

        QString selectedCall = callsignSelected();
        if(selectedCall.isEmpty()){
            return;
        }

        addMessageText(QString("%1 QTH %2").arg(selectedCall).arg(m_config.my_qth()), true);

        if(m_config.transmit_directed()) toggleTx(true);
    });

    auto grid = m_config.my_grid();
    auto gridAction = menu->addAction(QString("%1 GRID %2 - Send my current station Maidenhead grid locator").arg(call).arg(grid).trimmed());
    connect(gridAction, &QAction::triggered, this, [this](){

        QString selectedCall = callsignSelected();
        if(selectedCall.isEmpty()){
            return;
        }

        addMessageText(QString("%1 GRID %2").arg(selectedCall).arg(m_config.my_grid()), true);

        if(m_config.transmit_directed()) toggleTx(true);
    });


    menu->addSeparator();

    auto agnAction = menu->addAction(QString("%1 AGN? - Please repeat your last transmission").arg(call).trimmed());
    connect(agnAction, &QAction::triggered, this, [this](){

        QString selectedCall = callsignSelected();
        if(selectedCall.isEmpty()){
            return;
        }

        addMessageText(QString("%1 AGN?").arg(selectedCall), true);

        if(m_config.transmit_directed()) toggleTx(true);
    });

    auto qslQueryAction = menu->addAction(QString("%1 QSL? - Did you receive my last transmission?").arg(call).trimmed());
    connect(qslQueryAction, &QAction::triggered, this, [this](){

        QString selectedCall = callsignSelected();
        if(selectedCall.isEmpty()){
            return;
        }

        addMessageText(QString("%1 QSL?").arg(selectedCall), true);

        if(m_config.transmit_directed()) toggleTx(true);
    });

    auto qslAction = menu->addAction(QString("%1 QSL - I confirm I received your last transmission").arg(call).trimmed());
    connect(qslAction, &QAction::triggered, this, [this](){

        QString selectedCall = callsignSelected();
        if(selectedCall.isEmpty()){
            return;
        }

        addMessageText(QString("%1 QSL").arg(selectedCall), true);

        if(m_config.transmit_directed()) toggleTx(true);
    });

    auto yesAction = menu->addAction(QString("%1 YES - I confirm your last inquiry").arg(call).trimmed());
    connect(yesAction, &QAction::triggered, this, [this](){

        QString selectedCall = callsignSelected();
        if(selectedCall.isEmpty()){
            return;
        }

        addMessageText(QString("%1 YES").arg(selectedCall), true);

        if(m_config.transmit_directed()) toggleTx(true);
    });

    auto noAction = menu->addAction(QString("%1 NO - I do not confirm your last inquiry").arg(call).trimmed());
    connect(noAction, &QAction::triggered, this, [this](){

        QString selectedCall = callsignSelected();
        if(selectedCall.isEmpty()){
            return;
        }

        addMessageText(QString("%1 NO").arg(selectedCall), true);

        if(m_config.transmit_directed()) toggleTx(true);
    });

    auto hwAction = menu->addAction(QString("%1 HW CPY? - How do you copy?").arg(call).trimmed());
    connect(hwAction, &QAction::triggered, this, [this](){

        QString selectedCall = callsignSelected();
        if(selectedCall.isEmpty()){
            return;
        }

        addMessageText(QString("%1 HW CPY?").arg(selectedCall), true);

        if(m_config.transmit_directed()) toggleTx(true);
    });

    auto rrAction = menu->addAction(QString("%1 RR - Roger. Received. I copy.").arg(call).trimmed());
    connect(rrAction, &QAction::triggered, this, [this](){

        QString selectedCall = callsignSelected();
        if(selectedCall.isEmpty()){
            return;
        }

        addMessageText(QString("%1 RR").arg(selectedCall), true);

        if(m_config.transmit_directed()) toggleTx(true);
    });

    auto fbAction = menu->addAction(QString("%1 FB - Fine Business").arg(call).trimmed());
    connect(fbAction, &QAction::triggered, this, [this](){

        QString selectedCall = callsignSelected();
        if(selectedCall.isEmpty()){
            return;
        }

        addMessageText(QString("%1 FB").arg(selectedCall), true);

        if(m_config.transmit_directed()) toggleTx(true);
    });


    auto sevenThreeAction = menu->addAction(QString("%1 73 - I send my best regards / end of contact").arg(call).trimmed());
    connect(sevenThreeAction, &QAction::triggered, this, [this](){

        QString selectedCall = callsignSelected();
        if(selectedCall.isEmpty()){
            return;
        }

        addMessageText(QString("%1 73").arg(selectedCall), true);

        if(m_config.transmit_directed()) toggleTx(true);
    });
}

void MainWindow::on_queryButton_pressed(){
    QMenu *menu = ui->queryButton->menu();
    if(!menu){
        menu = new QMenu(ui->queryButton);
    }
    menu->clear();

    buildQueryMenu(menu, callsignSelected());

    ui->queryButton->setMenu(menu);
    ui->queryButton->showMenu();
}

void MainWindow::on_macrosMacroButton_pressed(){
    if(m_config.macros()->stringList().isEmpty()){
        on_actionSettings_triggered();
        return;
    }

    QMenu *menu = ui->macrosMacroButton->menu();
    if(!menu){
        menu = new QMenu(ui->macrosMacroButton);
    }
    menu->clear();

    foreach(QString macro, m_config.macros()->stringList()){
        QAction *action = menu->addAction(macro);
        connect(action, &QAction::triggered, this, [this, macro](){ addMessageText(macro); });
    }

    menu->addSeparator();
    auto action = new QAction(QIcon::fromTheme("edit-edit"), QString("Edit"), menu);
    menu->addAction(action);
    connect(action, &QAction::triggered, this, &MainWindow::on_actionSettings_triggered);

    ui->macrosMacroButton->setMenu(menu);
    ui->macrosMacroButton->showMenu();
}


void MainWindow::on_tableWidgetRXAll_cellClicked(int row, int /*col*/){
    auto item = ui->tableWidgetRXAll->item(row, 0);
    int offset = item->text().toInt();

    setFreqOffsetForRestore(offset, false);

    ui->tableWidgetCalls->selectionModel()->select(
        ui->tableWidgetCalls->selectionModel()->selection(),
        QItemSelectionModel::Deselect);
}

void MainWindow::on_tableWidgetRXAll_cellDoubleClicked(int row, int col){
    on_tableWidgetRXAll_cellClicked(row, col);

    // TODO: jsherer - could also parse the messages for the last callsign?
    auto item = ui->tableWidgetRXAll->item(row, 0);
    int offset = item->text().toInt();

    // print the history in the main window...
    int activityAging = m_config.activity_aging();
    QDateTime now = QDateTime::currentDateTimeUtc();
    QDateTime firstActivity = now;
    QString activityText;
    bool isLast = false;
    foreach(auto d, m_bandActivity[offset]){
        if(activityAging && d.utcTimestamp.secsTo(now)/60 >= activityAging){
            continue;
        }
        if(activityText.isEmpty()){
            firstActivity = d.utcTimestamp;
        }
        activityText.append(d.text);

        isLast = (d.bits & Varicode::FT8CallLast) == Varicode::FT8CallLast;
        if(isLast){
            // can also use \u0004 \u2666 \u2404
            activityText.append(" \u2301 ");
        }
    }
    if(!activityText.isEmpty()){
        displayTextForFreq(activityText, offset, firstActivity, false, true, isLast);
    }
}

void MainWindow::on_tableWidgetRXAll_selectionChanged(const QItemSelection &/*selected*/, const QItemSelection &/*deselected*/){
    on_extFreeTextMsgEdit_currentTextChanged(ui->extFreeTextMsgEdit->toPlainText());

    auto placeholderText = QString("Type your outgoing messages here.");
    auto selectedCall = callsignSelected();
    if(!selectedCall.isEmpty()){
        placeholderText = QString("Type your outgoing directed message to %1 here.").arg(selectedCall);
    }
    ui->extFreeTextMsgEdit->setPlaceholderText(placeholderText);

    updateButtonDisplay();
}

void MainWindow::on_tableWidgetCalls_cellClicked(int /*row*/, int /*col*/){
    auto call = callsignSelected();

    if(!m_callActivity.contains(call)){
        return;
    }

    auto d = m_callActivity[call];
    setFreqOffsetForRestore(d.freq, false);

    ui->tableWidgetRXAll->selectionModel()->select(
        ui->tableWidgetRXAll->selectionModel()->selection(),
        QItemSelectionModel::Deselect);
}

void MainWindow::on_tableWidgetCalls_cellDoubleClicked(int row, int col){
    on_tableWidgetCalls_cellClicked(row, col);

    auto call = callsignSelected();
    addMessageText(call);
}

void MainWindow::on_tableWidgetCalls_selectionChanged(const QItemSelection &selected, const QItemSelection &deselected){
    on_tableWidgetRXAll_selectionChanged(selected, deselected);
}

void MainWindow::on_freeTextMsg_currentTextChanged (QString const& text)
{
  msgtype(text, ui->freeTextMsg->lineEdit ());
}

void MainWindow::on_rptSpinBox_valueChanged(int n)
{
  int step=ui->rptSpinBox->singleStep();
  if(n%step !=0) {
    n++;
    ui->rptSpinBox->setValue(n);
  }
  m_rpt=QString::number(n);
  int ntx0=m_ntx;
  genStdMsgs(m_rpt);
  m_ntx=ntx0;
  if(m_ntx==1) ui->txrb1->setChecked(true);
  if(m_ntx==2) ui->txrb2->setChecked(true);
  if(m_ntx==3) ui->txrb3->setChecked(true);
  if(m_ntx==4) ui->txrb4->setChecked(true);
  if(m_ntx==5) ui->txrb5->setChecked(true);
  if(m_ntx==6) ui->txrb6->setChecked(true);
  statusChanged();
}

void MainWindow::on_tuneButton_clicked (bool checked)
{
  static bool lastChecked = false;
  if (lastChecked == checked) return;
  lastChecked = checked;
  QString curBand = ui->bandComboBox->currentText();
  if (checked && m_tune==false) { // we're starting tuning so remember Tx and change pwr to Tune value
    if (m_config.pwrBandTuneMemory ()) {
      m_pwrBandTxMemory[curBand] = ui->outAttenuation->value(); // remember our Tx pwr
      m_PwrBandSetOK = false;
      if (m_pwrBandTuneMemory.contains(curBand)) {
        ui->outAttenuation->setValue(m_pwrBandTuneMemory[curBand].toInt()); // set to Tune pwr
      }
      m_PwrBandSetOK = true;
    }
  }
  else { // we're turning off so remember our Tune pwr setting and reset to Tx pwr
    if (m_config.pwrBandTuneMemory() || m_config.pwrBandTxMemory()) {
      m_pwrBandTuneMemory[curBand] = ui->outAttenuation->value(); // remember our Tune pwr
      m_PwrBandSetOK = false;
      ui->outAttenuation->setValue(m_pwrBandTxMemory[curBand].toInt()); // set to Tx pwr
      m_PwrBandSetOK = true;
    }
  }
  if (m_tune) {
    tuneButtonTimer.start(250);
  } else {
    m_sentFirst73=false;
    itone[0]=0;
    on_monitorButton_clicked (true);
    m_tune=true;
  }
  Q_EMIT tune (checked);
}

void MainWindow::stop_tuning ()
{
  on_tuneButton_clicked(false);
  ui->tuneButton->setChecked (false);
  m_bTxTime=false;
  m_tune=false;
}

void MainWindow::stopTuneATU()
{
  on_tuneButton_clicked(false);
  m_bTxTime=false;
}

void MainWindow::on_monitorTxButton_clicked(){
    on_stopTxButton_clicked();
}

void MainWindow::on_stopTxButton_clicked()                    //Stop Tx
{
  if (m_tune) stop_tuning ();
  if (m_auto and !m_tuneup) auto_tx_mode (false);
  m_btxok=false;
  m_bCallingCQ = false;
  m_bAutoReply = false;         // ready for next
  ui->cbFirst->setStyleSheet ("");

  resetMessage();
}

void MainWindow::rigOpen ()
{
  update_dynamic_property (ui->readFreq, "state", "warning");
  ui->readFreq->setText ("CAT");
  ui->readFreq->setEnabled (true);
  m_config.transceiver_online ();
  Q_EMIT m_config.sync_transceiver (true, true);
}

void MainWindow::on_pbR2T_clicked()
{
  ui->TxFreqSpinBox->setValue(ui->RxFreqSpinBox->value ());
}

void MainWindow::on_pbT2R_clicked()
{
  if (ui->RxFreqSpinBox->isEnabled ())
    {
      ui->RxFreqSpinBox->setValue (ui->TxFreqSpinBox->value ());
    }
}

void MainWindow::on_beaconButton_clicked()
{
    if(ui->beaconButton->isChecked()){
        scheduleBacon(true);
    } else {
        pauseBacon();
    }
}

void MainWindow::on_readFreq_clicked()
{
  if (m_transmitting) return;

  if (m_config.transceiver_online ())
    {
      Q_EMIT m_config.sync_transceiver (true, true);
    }
}

void MainWindow::on_pbTxMode_clicked()
{
  if(m_modeTx=="JT9") {
    m_modeTx="JT65";
    ui->pbTxMode->setText("Tx JT65  #");
  } else {
    m_modeTx="JT9";
    ui->pbTxMode->setText("Tx JT9  @");
  }
  m_wideGraph->setModeTx(m_modeTx);
  statusChanged();
}

void MainWindow::setXIT(int n, Frequency base)
{
  if (m_transmitting && !m_config.tx_QSY_allowed ()) return;
  // If "CQ nnn ..." feature is active, set the proper Tx frequency
  if(m_config.split_mode () && ui->cbCQTx->isEnabled () && ui->cbCQTx->isVisible () &&
     ui->cbCQTx->isChecked())
    {
      if (6 == m_ntx || (7 == m_ntx && m_gen_message_is_cq))
        {
          // All conditions are met, use calling frequency
          base = m_freqNominal / 1000000 * 1000000 + 1000 * ui->sbCQTxFreq->value () + m_XIT;
        }
    }
  if (!base) base = m_freqNominal;
  m_XIT = 0;
  if (!m_bSimplex) {
    // m_bSimplex is false, so we can use split mode if requested
    if (m_config.split_mode () && (!m_config.enable_VHF_features () || m_mode == "FT8")) {
      // Don't use XIT for VHF & up
      m_XIT=(n/500)*500 - 1500;
    }

    if ((m_monitoring || m_transmitting)
        && m_config.is_transceiver_online ()
        && m_config.split_mode ())
      {
        // All conditions are met, reset the transceiver Tx dial
        // frequency
        m_freqTxNominal = base + m_XIT;
        if (m_astroWidget) m_astroWidget->nominal_frequency (m_freqNominal, m_freqTxNominal);
        Q_EMIT m_config.transceiver_tx_frequency (m_freqTxNominal + m_astroCorrection.tx);
      }
  }
  //Now set the audio Tx freq
  Q_EMIT transmitFrequency (ui->TxFreqSpinBox->value () - m_XIT);
}

void MainWindow::setFreqOffsetForRestore(int freq, bool shouldRestore){
    setFreq4(freq, freq);
    if(shouldRestore){
        m_shouldRestoreFreq = true;
    } else {
        m_previousFreq = 0;
        m_shouldRestoreFreq = false;
    }
}

bool MainWindow::tryRestoreFreqOffset(){
    if(!m_shouldRestoreFreq || m_previousFreq == 0){
        return false;
    }

    setFreqOffsetForRestore(m_previousFreq, false);

    return true;
}

void MainWindow::setFreq4(int rxFreq, int txFreq)
{
  // don't allow QSY if we've already queued a transmission
  if(isMessageQueuedForTransmit()){
      return;
  }

  if(rxFreq != txFreq){
      txFreq = rxFreq;
  }

  rxFreq = max(500, rxFreq);
  txFreq = max(500, txFreq);

  m_previousFreq = currentFreqOffset();

  if (ui->RxFreqSpinBox->isEnabled ()) ui->RxFreqSpinBox->setValue(rxFreq);
  ui->labDialFreqOffset->setText(QString("%1 Hz").arg(rxFreq));
  if(m_mode.startsWith ("WSPR")) {
    ui->WSPRfreqSpinBox->setValue(txFreq);
  } else {
    if (ui->TxFreqSpinBox->isEnabled ()) {
      ui->TxFreqSpinBox->setValue(txFreq);
    }
    else if (m_config.enable_VHF_features ()
             && (Qt::ControlModifier & QApplication::keyboardModifiers ())) {
      // for VHF & up we adjust Tx dial frequency to equalize Tx to Rx
      // when user CTRL+clicks on waterfall
      auto temp = ui->TxFreqSpinBox->value ();
      ui->RxFreqSpinBox->setValue (temp);
      setRig (m_freqNominal + txFreq - temp);
      setXIT (ui->TxFreqSpinBox->value ());
    }
  }


}

void MainWindow::handle_transceiver_update (Transceiver::TransceiverState const& s)
{
  // qDebug () << "MainWindow::handle_transceiver_update:" << s;
  Transceiver::TransceiverState old_state {m_rigState};
  //transmitDisplay (s.ptt ());
  if (s.ptt () && !m_rigState.ptt ()) // safe to start audio
                                      // (caveat - DX Lab Suite Commander)
    {
      if (m_tx_when_ready && g_iptt) // waiting to Tx and still needed
        {
          ptt1Timer.start(1000 * m_config.txDelay ()); //Start-of-transmission sequencer delay
        }
      m_tx_when_ready = false;
    }
  m_rigState = s;
  auto old_freqNominal = m_freqNominal;
  if (!old_freqNominal)
    {
      // always take initial rig frequency to avoid start up problems
      // with bogus Tx frequencies
      m_freqNominal = s.frequency ();
    }
  if (old_state.online () == false && s.online () == true)
    {
      // initializing
      on_monitorButton_clicked (!m_config.monitor_off_at_startup ());

      ui->autoReplyButton->setChecked(!m_config.autoreply_off_at_startup());
    }
  if (s.frequency () != old_state.frequency () || s.split () != m_splitMode)
    {
      m_splitMode = s.split ();
      if (!s.ptt ())
        {
          m_freqNominal = s.frequency () - m_astroCorrection.rx;
          if (old_freqNominal != m_freqNominal)
            {
              m_freqTxNominal = m_freqNominal;
              genCQMsg ();
            }

          if (m_monitoring)
            {
              m_lastMonitoredFrequency = m_freqNominal;
            }
          if (m_lastDialFreq != m_freqNominal &&
              (m_mode != "MSK144"
               || !(ui->cbCQTx->isEnabled () && ui->cbCQTx->isVisible () && ui->cbCQTx->isChecked()))) {

            m_lastDialFreq = m_freqNominal;
            m_secBandChanged=QDateTime::currentMSecsSinceEpoch()/1000;

            if(m_freqNominal != m_bandHoppedFreq){
                m_bandHopped = false;
            }

            if(s.frequency () < 30000000u && !m_mode.startsWith ("WSPR")) {
              // Write freq changes to ALL.TXT only below 30 MHz.
              QFile f2 {m_config.writeable_data_dir ().absoluteFilePath ("ALL.TXT")};
              if (f2.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
                QTextStream out(&f2);
                out << QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd hh:mm")
                    << "  " << qSetRealNumberPrecision (12) << (m_freqNominal / 1.e6) << " MHz  "
                    << m_mode << endl;
                f2.close();
              } else {
                MessageBox::warning_message (this, tr ("File Error")
                                             ,tr ("Cannot open \"%1\" for append: %2")
                                             .arg (f2.fileName ()).arg (f2.errorString ()));
              }
            }

            if (m_config.spot_to_reporting_networks ()) {
              pskSetLocal();
              aprsSetLocal();
            }
            statusChanged();
            m_wideGraph->setDialFreq(m_freqNominal / 1.e6);
          }
      } else {
        m_freqTxNominal = s.split () ? s.tx_frequency () - m_astroCorrection.tx : s.frequency ();
      }
      if (m_astroWidget) m_astroWidget->nominal_frequency (m_freqNominal, m_freqTxNominal);
  }
  // ensure frequency display is correct
  if (m_astroWidget && old_state.ptt () != s.ptt ()) setRig ();

  displayDialFrequency ();
  update_dynamic_property (ui->readFreq, "state", "ok");
  ui->readFreq->setEnabled (false);
  ui->readFreq->setText (s.split () ? "CAT/S" : "CAT");
}

void MainWindow::handle_transceiver_failure (QString const& reason)
{
  update_dynamic_property (ui->readFreq, "state", "error");
  ui->readFreq->setEnabled (true);
  on_stopTxButton_clicked ();
  rigFailure (reason);
}

void MainWindow::rigFailure (QString const& reason)
{
  if (m_first_error)
    {
      // one automatic retry
      QTimer::singleShot (0, this, SLOT (rigOpen ()));
      m_first_error = false;
    }
  else
    {
      if (m_splash && m_splash->isVisible ()) m_splash->hide ();
      m_rigErrorMessageBox.setDetailedText (reason);

      // don't call slot functions directly to avoid recursion
      m_rigErrorMessageBox.exec ();
      auto const clicked_button = m_rigErrorMessageBox.clickedButton ();
      if (clicked_button == m_configurations_button)
        {
          ui->menuConfig->exec (QCursor::pos ());
        }
      else
        {
          switch (m_rigErrorMessageBox.standardButton (clicked_button))
            {
            case MessageBox::Ok:
              m_config.select_tab (1);
              QTimer::singleShot (0, this, SLOT (on_actionSettings_triggered ()));
              break;

            case MessageBox::Retry:
              QTimer::singleShot (0, this, SLOT (rigOpen ()));
              break;

            case MessageBox::Cancel:
              QTimer::singleShot (0, this, SLOT (close ()));
              break;

            default: break;     // squashing compile warnings
            }
        }
      m_first_error = true;     // reset
    }
}

void MainWindow::transmit (double snr)
{
  double toneSpacing=0.0;
  if (m_modeTx == "JT65") {
    if(m_nSubMode==0) toneSpacing=11025.0/4096.0;
    if(m_nSubMode==1) toneSpacing=2*11025.0/4096.0;
    if(m_nSubMode==2) toneSpacing=4*11025.0/4096.0;
    Q_EMIT sendMessage (NUM_JT65_SYMBOLS,
           4096.0*12000.0/11025.0, ui->TxFreqSpinBox->value () - m_XIT,
           toneSpacing, m_soundOutput, m_config.audio_output_channel (),
           true, false, snr, m_TRperiod);
  }

  if (m_modeTx == "FT8") {
    toneSpacing=12000.0/1920.0;
    if(m_config.x2ToneSpacing()) toneSpacing=2*12000.0/1920.0;
    if(m_config.x4ToneSpacing()) toneSpacing=4*12000.0/1920.0;
    if(m_config.bFox() and !m_tune) toneSpacing=-1;
    Q_EMIT sendMessage (NUM_FT8_SYMBOLS,
           1920.0, ui->TxFreqSpinBox->value () - m_XIT,
           toneSpacing, m_soundOutput, m_config.audio_output_channel (),
           true, false, snr, m_TRperiod);
  }

  if (m_modeTx == "QRA64") {
    if(m_nSubMode==0) toneSpacing=12000.0/6912.0;
    if(m_nSubMode==1) toneSpacing=2*12000.0/6912.0;
    if(m_nSubMode==2) toneSpacing=4*12000.0/6912.0;
    if(m_nSubMode==3) toneSpacing=8*12000.0/6912.0;
    if(m_nSubMode==4) toneSpacing=16*12000.0/6912.0;
    Q_EMIT sendMessage (NUM_QRA64_SYMBOLS,
           6912.0, ui->TxFreqSpinBox->value () - m_XIT,
           toneSpacing, m_soundOutput, m_config.audio_output_channel (),
           true, false, snr, m_TRperiod);
  }

  if (m_modeTx == "JT9") {
    int nsub=pow(2,m_nSubMode);
    int nsps[]={480,240,120,60};
    double sps=m_nsps;
    m_toneSpacing=nsub*12000.0/6912.0;
    if(m_config.x2ToneSpacing()) m_toneSpacing=2.0*m_toneSpacing;
    if(m_config.x4ToneSpacing()) m_toneSpacing=4.0*m_toneSpacing;
    bool fastmode=false;
    if(m_bFast9 and (m_nSubMode>=4)) {
      fastmode=true;
      sps=nsps[m_nSubMode-4];
      m_toneSpacing=12000.0/sps;
    }
    Q_EMIT sendMessage (NUM_JT9_SYMBOLS, sps,
                        ui->TxFreqSpinBox->value() - m_XIT, m_toneSpacing,
                        m_soundOutput, m_config.audio_output_channel (),
                        true, fastmode, snr, m_TRperiod);
  }

  if (m_modeTx == "MSK144") {
    m_nsps=6;
    double f0=1000.0;
    if(!m_bFastMode) {
      m_nsps=192;
      f0=ui->TxFreqSpinBox->value () - m_XIT - 0.5*m_toneSpacing;
    }
    m_toneSpacing=6000.0/m_nsps;
    m_FFTSize = 7 * 512;
    Q_EMIT FFTSize (m_FFTSize);
    int nsym;
    nsym=NUM_MSK144_SYMBOLS;
    if(itone[40] < 0) nsym=40;
    Q_EMIT sendMessage (nsym, double(m_nsps), f0, m_toneSpacing,
                        m_soundOutput, m_config.audio_output_channel (),
                        true, true, snr, m_TRperiod);
  }

  if (m_modeTx == "JT4") {
    if(m_nSubMode==0) toneSpacing=4.375;
    if(m_nSubMode==1) toneSpacing=2*4.375;
    if(m_nSubMode==2) toneSpacing=4*4.375;
    if(m_nSubMode==3) toneSpacing=9*4.375;
    if(m_nSubMode==4) toneSpacing=18*4.375;
    if(m_nSubMode==5) toneSpacing=36*4.375;
    if(m_nSubMode==6) toneSpacing=72*4.375;
    Q_EMIT sendMessage (NUM_JT4_SYMBOLS,
           2520.0*12000.0/11025.0, ui->TxFreqSpinBox->value () - m_XIT,
           toneSpacing, m_soundOutput, m_config.audio_output_channel (),
           true, false, snr, m_TRperiod);
  }
  if (m_mode=="WSPR") {
    int nToneSpacing=1;
    if(m_config.x2ToneSpacing()) nToneSpacing=2;
    if(m_config.x4ToneSpacing()) nToneSpacing=4;
    Q_EMIT sendMessage (NUM_WSPR_SYMBOLS, 8192.0,
                        ui->TxFreqSpinBox->value() - 1.5 * 12000 / 8192,
                        m_toneSpacing*nToneSpacing, m_soundOutput,
                        m_config.audio_output_channel(),true, false, snr,
                        m_TRperiod);
  }
  if (m_mode=="WSPR-LF") {
    Q_EMIT sendMessage (NUM_WSPR_LF_SYMBOLS, 24576.0,
                        ui->TxFreqSpinBox->value(),
                        m_toneSpacing, m_soundOutput,
                        m_config.audio_output_channel(),true, false, snr,
                        m_TRperiod);
  }
  if(m_mode=="Echo") {
    //??? should use "fastMode = true" here ???
    Q_EMIT sendMessage (27, 1024.0, 1500.0, 0.0, m_soundOutput,
                        m_config.audio_output_channel(),
                        false, false, snr, m_TRperiod);
  }

  if(m_mode=="ISCAT") {
    double sps,f0;
    if(m_nSubMode==0) {
      sps=512.0*12000.0/11025.0;
      toneSpacing=11025.0/512.0;
      f0=47*toneSpacing;
    } else {
      sps=256.0*12000.0/11025.0;
      toneSpacing=11025.0/256.0;
      f0=13*toneSpacing;
    }
    Q_EMIT sendMessage (NUM_ISCAT_SYMBOLS, sps, f0, toneSpacing, m_soundOutput,
                        m_config.audio_output_channel(),
                        true, true, snr, m_TRperiod);
  }

// In auto-sequencing mode, stop after 5 transmissions of "73" message.
  if (m_bFastMode || m_bFast9) {
    if (ui->cbAutoSeq->isVisible () && ui->cbAutoSeq->isChecked ()) {
      if(m_ntx==5) {
        m_nTx73 += 1;
      } else {
        m_nTx73=0;
      }
    }
  }
}

void MainWindow::on_outAttenuation_valueChanged (int a)
{
  QString tt_str;
  qreal dBAttn {a / 10.};       // slider interpreted as dB / 100
  if (m_tune && m_config.pwrBandTuneMemory()) {
    tt_str = tr ("Tune digital gain ");
  } else {
    tt_str = tr ("Transmit digital gain ");
  }
  tt_str += (a ? QString::number (-dBAttn, 'f', 1) : "0") + "dB";
  if (!m_block_pwr_tooltip) {
    QToolTip::showText (QCursor::pos (), tt_str, ui->outAttenuation);
  }
  QString curBand = ui->bandComboBox->currentText();
  if (m_PwrBandSetOK && !m_tune && m_config.pwrBandTxMemory ()) {
    m_pwrBandTxMemory[curBand] = a; // remember our Tx pwr
  }
  if (m_PwrBandSetOK && m_tune && m_config.pwrBandTuneMemory()) {
    m_pwrBandTuneMemory[curBand] = a; // remember our Tune pwr
  }
  Q_EMIT outAttenuationChanged (dBAttn);
}

void MainWindow::on_actionShort_list_of_add_on_prefixes_and_suffixes_triggered()
{
  if (!m_prefixes) {
    m_prefixes.reset (new HelpTextWindow {tr ("Prefixes"), ":/prefixes.txt", {"Courier", 10}});
  }
  m_prefixes->showNormal();
  m_prefixes->raise ();
}

bool MainWindow::shortList(QString callsign)
{
  int n=callsign.length();
  int i1=callsign.indexOf("/");
  Q_ASSERT(i1>0 and i1<n);
  QString t1=callsign.mid(0,i1);
  QString t2=callsign.mid(i1+1,n-i1-1);
  bool b=(m_pfx.contains(t1) or m_sfx.contains(t2));
  return b;
}

void MainWindow::pskSetLocal ()
{
  psk_Reporter->setLocalStation(m_config.my_callsign (), m_config.my_grid (),
        m_config.my_station(), QString {"FT8Call v" + version() }.simplified ());
}

void MainWindow::aprsSetLocal ()
{
    auto ssid = m_config.aprs_ssid();
    auto call = Radio::base_callsign(m_config.my_callsign());
    if(!ssid.isEmpty()){
        if(!ssid.startsWith("-")){
            ssid = "-" + ssid;
        }
        call = call + ssid;
    }
    m_aprsClient->setLocalStation(call, m_config.my_grid());
}

void MainWindow::transmitDisplay (bool transmitting)
{
  if (transmitting == m_transmitting) {
    if (transmitting) {
      ui->signal_meter_widget->setValue(0,0);
      if (m_monitoring) monitor (false);
      m_btxok=true;
    }

    auto QSY_allowed = !transmitting or m_config.tx_QSY_allowed () or
      !m_config.split_mode ();
    if (ui->cbHoldTxFreq->isChecked ()) {
      ui->RxFreqSpinBox->setEnabled (QSY_allowed);
      ui->pbT2R->setEnabled (QSY_allowed);
    }

    if (!m_mode.startsWith ("WSPR")) {
      if(m_config.enable_VHF_features ()) {
//### During tests, at least, allow use of Tx Freq spinner with VHF features enabled.
        // used fixed 1000Hz Tx DF for VHF & up QSO modes
//        ui->TxFreqSpinBox->setValue(1000);
//        ui->TxFreqSpinBox->setEnabled (false);
        ui->TxFreqSpinBox->setEnabled (true);
//###
      } else {
        ui->TxFreqSpinBox->setEnabled (QSY_allowed and !m_bFastMode);
        ui->pbR2T->setEnabled (QSY_allowed);
        ui->cbHoldTxFreq->setEnabled (QSY_allowed);
      }
    }

    // the following are always disallowed in transmit
    //ui->menuMode->setEnabled (!transmitting);
    //ui->bandComboBox->setEnabled (!transmitting);
    if (!transmitting) {
      if (m_mode == "JT9+JT65") {
        // allow mode switch in Rx when in dual mode
        ui->pbTxMode->setEnabled (true);
      }
    } else {
      ui->pbTxMode->setEnabled (false);
    }
  }

  // TODO: jsherer - encapsulate this in a function?
  ui->monitorButton->setVisible(!transmitting);
  ui->monitorTxButton->setVisible(transmitting);
}

void MainWindow::on_sbFtol_valueChanged(int value)
{
  m_wideGraph->setTol (value);
}

void::MainWindow::VHF_features_enabled(bool b)
{
  if(m_mode!="JT4" and m_mode!="JT65") b=false;
  if(b and (ui->actionInclude_averaging->isChecked() or
             ui->actionInclude_correlation->isChecked())) {
    ui->actionDeepestDecode->setChecked (true);
  }
  ui->actionInclude_averaging->setVisible (b);
  ui->actionInclude_correlation->setVisible (b);
  ui->actionMessage_averaging->setEnabled(b);
  ui->actionEnable_AP_DXcall->setVisible (m_mode=="QRA64");
  ui->actionEnable_AP_JT65->setVisible (b && m_mode=="JT65");
  if(!b && m_msgAvgWidget and !m_config.bFox()) {
    if(m_msgAvgWidget->isVisible()) m_msgAvgWidget->close();
  }
}

void MainWindow::on_sbTR_valueChanged(int value)
{
//  if(!m_bFastMode and n>m_nSubMode) m_MinW=m_nSubMode;
  if(m_bFastMode or m_mode=="FreqCal") {
    m_TRperiod = value;
    m_fastGraph->setTRPeriod (value);
    m_modulator->setTRPeriod (value); // TODO - not thread safe
    m_detector->setTRPeriod (value);  // TODO - not thread safe
    m_wideGraph->setPeriod (value, m_nsps);
    progressBar.setMaximum (value);
  }
  if(m_monitoring) {
    on_stopButton_clicked();
    on_monitorButton_clicked(true);
  }
  if(m_transmitting) {
    on_stopTxButton_clicked();
  }
}

QChar MainWindow::current_submode () const
{
  QChar submode {0};
  if (m_mode.contains (QRegularExpression {R"(^(JT65|JT9|JT4|ISCAT|QRA64)$)"})
      && (m_config.enable_VHF_features () || "JT4" == m_mode || "ISCAT" == m_mode))
    {
      submode = m_nSubMode + 65;
    }
  return submode;
}

void MainWindow::on_sbSubmode_valueChanged(int n)
{
  m_nSubMode=n;
  m_wideGraph->setSubMode(m_nSubMode);
  auto submode = current_submode ();
  if (submode != QChar::Null)
    {
      mode_label.setText (m_mode + " " + submode);
    }
  else
    {
      mode_label.setText (m_mode);
    }
  if(m_mode=="ISCAT") {
    if(m_nSubMode==0) ui->TxFreqSpinBox->setValue(1012);
    if(m_nSubMode==1) ui->TxFreqSpinBox->setValue(560);
  }
  if(m_mode=="JT9") {
    if(m_nSubMode<4) {
      ui->cbFast9->setChecked(false);
      on_cbFast9_clicked(false);
      ui->cbFast9->setEnabled(false);
      ui->sbTR->setVisible(false);
      m_TRperiod=60;
    } else {
      ui->cbFast9->setEnabled(true);
    }
    ui->sbTR->setVisible(m_bFast9);
    if(m_bFast9) ui->TxFreqSpinBox->setValue(700);
  }
  if(m_transmitting and m_bFast9 and m_nSubMode>=4) transmit (99.0);
  statusUpdate ();
}

void MainWindow::on_cbFast9_clicked(bool b)
{
  if(m_mode=="JT9") {
    m_bFast9=b;
//    ui->cbAutoSeq->setVisible(b);
  }

  if(b) {
    m_TRperiod = ui->sbTR->value ();
  } else {
    m_TRperiod=60;
  }
  progressBar.setMaximum(m_TRperiod);
  m_wideGraph->setPeriod(m_TRperiod,m_nsps);
  fast_config(b);
  statusChanged ();
}


void MainWindow::on_cbShMsgs_toggled(bool b)
{
  ui->cbTx6->setEnabled(b);
  m_bShMsgs=b;
  if(b) ui->cbSWL->setChecked(false);
  if(m_bShMsgs and (m_mode=="MSK144")) ui->rptSpinBox->setValue(1);
  int itone0=itone[0];
  int ntx=m_ntx;
  m_lastCallsign.clear ();      // ensure Tx5 gets updated
  genStdMsgs(m_rpt);
  itone[0]=itone0;
  if(ntx==1) ui->txrb1->setChecked(true);
  if(ntx==2) ui->txrb2->setChecked(true);
  if(ntx==3) ui->txrb3->setChecked(true);
  if(ntx==4) ui->txrb4->setChecked(true);
  if(ntx==5) ui->txrb5->setChecked(true);
  if(ntx==6) ui->txrb6->setChecked(true);
}

void MainWindow::on_cbSWL_toggled(bool b)
{
  if(b) ui->cbShMsgs->setChecked(false);
}

void MainWindow::on_cbTx6_toggled(bool)
{
  genCQMsg ();
}

// Takes a decoded CQ line and sets it up for reply
void MainWindow::replyToCQ (QTime time, qint32 snr, float delta_time, quint32 delta_frequency
                            , QString const& mode, QString const& message_text
                            , bool /*low_confidence*/, quint8 modifiers)
{
  if (!m_config.accept_udp_requests ())
    {
      return;
    }

  QString format_string {"%1 %2 %3 %4 %5 %6"};
  auto const& time_string = time.toString ("~" == mode || "&" == mode ? "hhmmss" : "hhmm");
  auto cqtext = format_string
    .arg (time_string)
    .arg (snr, 3)
    .arg (delta_time, 4, 'f', 1)
    .arg (delta_frequency, 4)
    .arg (mode, -2)
    .arg (message_text);
  auto messages = ui->decodedTextBrowser->toPlainText ();
  auto position = messages.lastIndexOf (cqtext);
  if (position < 0)
    {
      // try again with with -0.0 delta time
      position = messages.lastIndexOf (format_string
                                       .arg (time_string)
                                       .arg (snr, 3)
                                       .arg ('-' + QString::number (delta_time, 'f', 1), 4)
                                       .arg (delta_frequency, 4)
                                       .arg (mode, -2)
                                       .arg (message_text));
    }
  if (position >= 0)
    {
      if (m_config.udpWindowToFront ())
        {
          show ();
          raise ();
          activateWindow ();
        }
      if (m_config.udpWindowRestore () && isMinimized ())
        {
          showNormal ();
          raise ();
        }
      // find the linefeed at the end of the line
      position = ui->decodedTextBrowser->toPlainText().indexOf(QChar::LineFeed,position);
      if (message_text.contains (QRegularExpression {R"(^(CQ |CQDX |QRZ ))"})) {
        // a message we are willing to accept and auto reply to
        m_bDoubleClicked = true;
      }
      auto start = messages.left (position).lastIndexOf (QChar::LineFeed) + 1;
      DecodedText message {messages.mid (start, position - start), ("MSK144" == m_mode || "FT8" == m_mode) &&
          ui->cbVHFcontest->isChecked(), m_config.my_grid ()};
      Qt::KeyboardModifiers kbmod {modifiers << 24};
      processMessage (message, kbmod);
      tx_watchdog (false);
      QApplication::alert (this);
    }
  else
    {
      qDebug () << "process reply message ignored, decode not found:" << cqtext;
    }
}

void MainWindow::locationChange (QString const& location)
{
  QString grid {location.trimmed ()};
  int len;

  // string 6 chars or fewer, interpret as a grid, or use with a 'GRID:' prefix
  if (grid.size () > 6) {
    if (grid.toUpper ().startsWith ("GRID:")) {
      grid = grid.mid (5).trimmed ();
    }
    else {
      // TODO - support any other formats, e.g. latlong? Or have that conversion done external to wsjtx
      return;
    }
  }
  if (MaidenheadLocatorValidator::Acceptable == MaidenheadLocatorValidator ().validate (grid, len)) {
    qDebug() << "locationChange: Grid supplied is " << grid;
    if (m_config.my_grid () != grid) {
      m_config.set_dynamic_location (grid);
      genStdMsgs (m_rpt, false);
      statusUpdate ();
    }
  } else {
    qDebug() << "locationChange: Invalid grid " << grid;
  }
}

void MainWindow::replayDecodes ()
{
  // we accept this request even if the setting to accept UDP requests
  // is not checked

  // attempt to parse the decoded text
  Q_FOREACH (auto const& message
             , ui->decodedTextBrowser->toPlainText ().split (QChar::LineFeed,
                                                             QString::SkipEmptyParts))
    {
      if (message.size() >= 4 && message.left (4) != "----")
        {
          auto const& parts = message.split (' ', QString::SkipEmptyParts);
          if (parts.size () >= 5 && parts[3].contains ('.')) // WSPR
            {
              postWSPRDecode (false, parts);
            }
          else
            {
              auto eom_pos = message.indexOf (' ', 35);
              // we always want at least the characters to position 35
              if (eom_pos < 35)
                {
                  eom_pos = message.size () - 1;
                }
              // TODO - how to skip ISCAT decodes
              postDecode (false, message.left (eom_pos + 1));
            }
        }
    }
  statusChanged ();
}

void MainWindow::postDecode (bool is_new, QString const& message)
{
#if 0
  auto const& decode = message.trimmed ();
  auto const& parts = decode.left (22).split (' ', QString::SkipEmptyParts);
  if (parts.size () >= 5)
  {
      auto has_seconds = parts[0].size () > 4;
      m_messageClient->decode (is_new
                               , QTime::fromString (parts[0], has_seconds ? "hhmmss" : "hhmm")
                               , parts[1].toInt ()
                               , parts[2].toFloat (), parts[3].toUInt (), parts[4]
                               , decode.mid (has_seconds ? 24 : 22, 21)
                               , QChar {'?'} == decode.mid (has_seconds ? 24 + 21 : 22 + 21, 1)
                               , m_diskData);
  }
#endif

  if(is_new){
      m_rxDirty = true;
  }
}

void MainWindow::displayTransmit(){
    updateButtonDisplay();
}

void MainWindow::updateButtonDisplay(){
    // Transmit Activity
    update_dynamic_property (ui->startTxButton, "transmitting", m_transmitting);

    bool isTransmitting = m_transmitting || m_txFrameCount > 0;
    bool emptyCallsign = callsignSelected().isEmpty();

    ui->cqMacroButton->setDisabled(isTransmitting);
    ui->replyMacroButton->setDisabled(isTransmitting || emptyCallsign);
    ui->macrosMacroButton->setDisabled(isTransmitting);
    ui->queryButton->setDisabled(isTransmitting || emptyCallsign);
}

QString MainWindow::callsignSelected(){
    if(!ui->tableWidgetCalls->selectedItems().isEmpty()){
        auto selectedCalls = ui->tableWidgetCalls->selectedItems();
        if(!selectedCalls.isEmpty()){
            auto call = selectedCalls.first()->data(Qt::UserRole).toString();
            if(!call.isEmpty()){
                return call;
            }
        }
    }

    if(!ui->tableWidgetRXAll->selectedItems().isEmpty()){
        int selectedOffset = -1;
        auto selectedItems = ui->tableWidgetRXAll->selectedItems();
        selectedOffset = selectedItems.first()->text().toInt();

        auto keys = m_callActivity.keys();
        qSort(keys.begin(), keys.end(), [this](QString const &a, QString const &b){
            auto tA = m_callActivity[a].utcTimestamp;
            auto tB = m_callActivity[b].utcTimestamp;
            if(tA == tB){
                return a < b;
            }
            return tB < tA;
        });
        foreach(auto call, keys){
            auto d = m_callActivity[call];
            if(d.freq == selectedOffset){
                return d.call;
            }
        }
    }

    auto text = ui->extFreeTextMsgEdit->toPlainText().left(11); // Maximum callsign is 6 + / + 4 = 11 characters
    auto calls = Varicode::parseCallsigns(text);
    if(!calls.isEmpty() && text.startsWith(calls.first()) && calls.first() != m_config.my_callsign()){
        return calls.first();
    }

    return QString();
}

bool MainWindow::isRecentOffset(int offset){
    if(abs(offset - currentFreqOffset()) <= 10){
        return true;
    }
    return (
        m_rxRecentCache.contains(offset/10*10) &&
        m_rxRecentCache[offset/10*10]->secsTo(QDateTime::currentDateTimeUtc()) < 120
    );
}

void MainWindow::markOffsetRecent(int offset){
    m_rxRecentCache.insert(offset/10*10, new QDateTime(QDateTime::currentDateTimeUtc()), 10);
    m_rxRecentCache.insert(offset/10*10+10, new QDateTime(QDateTime::currentDateTimeUtc()), 10);
}

bool MainWindow::isDirectedOffset(int offset, bool *pIsAllCall){
    bool isDirected = (
        m_rxDirectedCache.contains(offset/10*10) &&
        m_rxDirectedCache[offset/10*10]->date.secsTo(QDateTime::currentDateTimeUtc()) < 300
    );

    if(isDirected){
        if(pIsAllCall) *pIsAllCall = m_rxDirectedCache[offset/10*10]->isAllcall;
    }

    return isDirected;
}

void MainWindow::markOffsetDirected(int offset, bool isAllCall){
    CachedDirectedType *d1 = new CachedDirectedType{ isAllCall, QDateTime::currentDateTimeUtc() };
    CachedDirectedType *d2 = new CachedDirectedType{ isAllCall, QDateTime::currentDateTimeUtc() };
    m_rxDirectedCache.insert(offset/10*10,    d1, 10);
    m_rxDirectedCache.insert(offset/10*10+10, d2, 10);
}

bool MainWindow::isMyCallIncluded(const QString &text){
    QString myCall = Radio::base_callsign(m_config.my_callsign());

    if(myCall.isEmpty()){
        return false;
    }

    return text.contains(myCall);
}

bool MainWindow::isAllCallIncluded(const QString &text){
    return text.contains("ALLCALL");
}

void MainWindow::processActivity(bool force) {
    if (!m_rxDirty && !force) {
        return;
    }

    // Recent Rx Activity
    processRxActivity();

    // Grouped Compound Activity
    processCompoundActivity();

    // Buffered Activity
    processBufferedActivity();

    // Command Activity
    processCommandActivity();

    // Process PSKReporter Spots
    processSpots();

    m_rxDirty = false;
}

void MainWindow::processRxActivity() {
    if(m_rxActivityQueue.isEmpty()){
        return;
    }

    while (!m_rxActivityQueue.isEmpty()) {
        ActivityDetail d = m_rxActivityQueue.dequeue();

        // if this is a compound message or it's a directed message needing a compound call, skip.
        // these messages will be displayed when the compound calls come through
#if 0
        if(d.isCompound || (d.isDirected && d.text.contains("<....>"))){
#endif

        // if this is a _partial_ directed message, skip until the complete call comes through.
        if(d.isDirected && d.text.contains("<....>")){
            continue;
        }

        // use the actual frequency and check its delta from our current frequency
        // meaning, if our current offset is 1502 and the d.freq is 1492, the delta is <= 10;
        bool shouldDisplay = abs(d.freq - currentFreqOffset()) <= 10;

#if 0
        // if this is a recent non-directed offset, bump the cache and display...
        if(isRecentOffset(d.freq)){
            markOffsetRecent(d.freq);
            shouldDisplay = true;
        }
#endif

        // if this is a (recent) directed offset, bump the cache, and display...
        // this will allow a directed free text command followed by non-buffered data frames.
        bool isDirectedAllCall = false;
        if(isDirectedOffset(d.freq, &isDirectedAllCall)){
            markOffsetDirected(d.freq, isDirectedAllCall);
            shouldDisplay = true;
        }

        // TODO: jsherer - develop a better way to determine if we can display this band activity...
#if 0
        if(isRecentOffset(freq) || isAllCallIncluded(d.text)){
            m_rxRecentCache.insert(freq, new QDateTime(QDateTime::currentDateTimeUtc()), 25);
            shouldDisplay = true;
        }

        if(isDirectedOffset(freq) || isMyCallIncluded(d.text)){
            m_rxDirectedCache.insert(freq, new QDateTime(QDateTime::currentDateTimeUtc()), 25);
            shouldDisplay = true;
        }
#endif

        if(!shouldDisplay){
            continue;
        }

        // ok, we're good to display...let's cache that fact and then display!
        markOffsetRecent(d.freq);

        bool isFirst = (d.bits & Varicode::FT8CallFirst) == Varicode::FT8CallFirst;
        bool isLast = (d.bits & Varicode::FT8CallLast) == Varicode::FT8CallLast;

        // if we're the last message, let's display our EOT character
        if (isLast) {
            // can also use \u0004 \u2666 \u2404
            d.text = QString("%1 \u2301 ").arg(d.text);
        }

        // log it to the display!
        displayTextForFreq(d.text, d.freq, d.utcTimestamp, false, isFirst, isLast);

        if(isLast && !d.isBuffered){
            // buffered commands need the rxFrameBlockNumbers cache so it can fixup its display
            // all other "last" data frames can clear the rxFrameBlockNumbers cache so the next message will be on a new line.
            m_rxFrameBlockNumbers.remove(d.freq);
        }
    }
}

void MainWindow::processCompoundActivity() {
    if(m_messageBuffer.isEmpty()){
        return;
    }

    // group compound callsign and directed commands together.
    foreach(auto freq, m_messageBuffer.keys()) {
        QMap < int, MessageBuffer > ::iterator i = m_messageBuffer.find(freq);

        MessageBuffer & buffer = i.value();

        qDebug() << "-> grouping buffer for freq" << freq;

        if (buffer.compound.isEmpty()) {
            qDebug() << "-> buffer.compound is empty...skip";
            continue;
        }

        // if we don't have an initialized command, skip...
        int bits = buffer.cmd.bits;
        bool validBits = (
            bits == Varicode::FT8Call                                         ||
            ((bits & Varicode::FT8CallFirst)    == Varicode::FT8CallFirst)    ||
            ((bits & Varicode::FT8CallLast)     == Varicode::FT8CallLast)     ||
            ((bits & Varicode::FT8CallReserved) == Varicode::FT8CallReserved)
        );
        if (!validBits) {
            qDebug() << "-> buffer.cmd bits is invalid...skip";
            continue;
        }

        // if we need two compound calls, but less than two have arrived...skip
        if (buffer.cmd.from == "<....>" && buffer.cmd.to == "<....>" && buffer.compound.length() < 2) {
            qDebug() << "-> buffer needs two compound, but has less...skip";
            continue;
        }

        // if we need one compound call, but non have arrived...skip
        if ((buffer.cmd.from == "<....>" || buffer.cmd.to == "<....>") && buffer.compound.length() < 1) {
            qDebug() << "-> buffer needs one compound, but has less...skip";
            continue;
        }

        if (buffer.cmd.from == "<....>") {
            auto d = buffer.compound.dequeue();
            buffer.cmd.from = d.call;
            buffer.cmd.grid = d.grid;
            buffer.cmd.isCompound = true;
            buffer.cmd.utcTimestamp = qMin(buffer.cmd.utcTimestamp, d.utcTimestamp);

            if ((d.bits & Varicode::FT8CallLast) == Varicode::FT8CallLast) {
                buffer.cmd.bits = d.bits;
            }
        }

        if (buffer.cmd.to == "<....>") {
            auto d = buffer.compound.dequeue();
            buffer.cmd.to = d.call;
            buffer.cmd.isCompound = true;
            buffer.cmd.utcTimestamp = qMin(buffer.cmd.utcTimestamp, d.utcTimestamp);

            if ((d.bits & Varicode::FT8CallLast) == Varicode::FT8CallLast) {
                buffer.cmd.bits = d.bits;
            }
        }

        if ((buffer.cmd.bits & Varicode::FT8CallLast) != Varicode::FT8CallLast) {
            qDebug() << "-> still not last message...skip";
            continue;
        }

        // fixup the datetime with the "minimum" dt seen
        // this will allow us to delete the activity lines
        // when the compound buffered command comes in.
        auto dt = buffer.cmd.utcTimestamp;
        foreach(auto c, buffer.compound){
            dt = qMin(dt, c.utcTimestamp);
        }
        foreach(auto m, buffer.msgs){
            dt = qMin(dt, m.utcTimestamp);
        }
        buffer.cmd.utcTimestamp = dt;

        qDebug() << "buffered compound command ready" << buffer.cmd.from << buffer.cmd.to << buffer.cmd.cmd;

        m_rxCommandQueue.append(buffer.cmd);
        m_messageBuffer.remove(freq);
    }
}

void MainWindow::processBufferedActivity() {
    if(m_messageBuffer.isEmpty()){
        return;
    }

    foreach(auto freq, m_messageBuffer.keys()) {
        auto buffer = m_messageBuffer[freq];

        if (buffer.msgs.isEmpty()) {
            continue;
        }

        if ((buffer.msgs.last().bits & Varicode::FT8CallLast) != Varicode::FT8CallLast) {
            continue;
        }

        QString message;
        foreach(auto part, buffer.msgs) {
            message.append(part.text);
        }
        message = rstrip(message);

        QString checksum;

        bool valid = false;

        if(Varicode::isCommandBuffered(buffer.cmd.cmd)){
            int checksumSize = Varicode::isCommandChecksumed(buffer.cmd.cmd);

            if(checksumSize == 32) {
                checksum = message.right(6);
                message = message.left(message.length() - 7);
                valid = Varicode::checksum32Valid(checksum, message);
            } else if(checksumSize == 16) {
                checksum = message.right(3);
                message = message.left(message.length() - 4);
                valid = Varicode::checksum16Valid(checksum, message);
            } else if (checksumSize == 0) {
                valid = true;
            }
        } else {
            valid = true;
        }


        if (valid) {
            buffer.cmd.bits |= Varicode::FT8CallLast;
            buffer.cmd.text = message;
            buffer.cmd.isBuffered = true;
            m_rxCommandQueue.append(buffer.cmd);
        } else {
            qDebug() << "Buffered message failed checksum...discarding";
            qDebug() << "Checksum:" << checksum;
            qDebug() << "Message:" << message;
        }

        // regardless of valid or not, remove the "complete" buffered message from the buffer cache
        m_messageBuffer.remove(freq);
    }
}

void MainWindow::processCommandActivity() {
#if 0
    if (!m_txFrameQueue.isEmpty()) {
        return;
    }
#endif

    if (m_rxCommandQueue.isEmpty()) {
        return;
    }

#if 0
    bool processed = false;

    int f = currentFreq();
#endif

    // TODO: jsherer - should we, if we have _any_ directed messages, pause the beacon or maybe just bump it?
    // pauseBacon();

    while (!m_rxCommandQueue.isEmpty()) {
        auto d = m_rxCommandQueue.dequeue();

        bool isAllCall = isAllCallIncluded(d.to);

        qDebug() << "try processing command" << d.from << d.to << d.cmd << d.freq;

        // if we need a compound callsign but never got one...skip
        if (d.from == "<....>" || d.to == "<....>") {
            continue;
        }

        // we're only processing a subset of queries at this point
        if (!Varicode::isCommandAllowed(d.cmd)) {
            continue;
        }

        // log call activity...
        CallDetail cd = {};
        cd.call = d.from;
        cd.grid = d.grid;
        cd.snr = d.snr;
        cd.freq = d.freq;
        cd.bits = d.bits;
        cd.utcTimestamp = d.utcTimestamp;
        logCallActivity(cd, true);


        // we're only responding to allcall and our callsign at this point, so we'll end after logging the callsigns we've heard
        if (!isAllCall && d.to != m_config.my_callsign().trimmed() && d.to != Radio::base_callsign(m_config.my_callsign()).trimmed()) {
            continue;
        }

        // if this is an allcall, check to make sure we haven't replied to their allcall recently (in the past beacon interval)
        // that way we never get spammed by allcalls at a high frequency than what we would beacon
        if (isAllCall){
            if(m_txAllcallCommandCache.contains(d.from) && m_txAllcallCommandCache[d.from]->secsTo(QDateTime::currentDateTimeUtc()) / 60 < m_config.beacon()) {
                continue;
            }

            m_txAllcallCommandCache.insert(d.from, new QDateTime(QDateTime::currentDateTimeUtc()), 25);
        }

        // display the command activity
        ActivityDetail ad = {};
        ad.isLowConfidence = false;
        ad.isFree = true;
        ad.isDirected = true;
        ad.bits = d.bits;
        ad.freq = d.freq;
        ad.snr = d.snr;
        ad.text = QString("%1: %2%3 ").arg(d.from).arg(d.to).arg(d.cmd);
        if(!d.extra.isEmpty()){
            ad.text += d.extra;
        }
        if(!d.text.isEmpty()){
            ad.text += d.text;
        }
        bool isLast = (ad.bits & Varicode::FT8CallLast) == Varicode::FT8CallLast;
        if (isLast) {
            // can also use \u0004 \u2666 \u2404
            ad.text += QString(" \u2301 ");
        }
        ad.utcTimestamp = d.utcTimestamp;


        // we'd be double printing here if were on frequency, so let's be "smart" about this...
        bool shouldDisplay = true;
        if(shouldDisplay){
            auto c = ui->textEditRX->textCursor();
            c.movePosition(QTextCursor::End);
            ui->textEditRX->setTextCursor(c);

            if(isRecentOffset(d.freq) && ui->textEditRX->find(d.utcTimestamp.time().toString(), QTextDocument::FindBackward)){
                // ... maybe we could delete the last line that had this message on this frequency...
                c = ui->textEditRX->textCursor();
                c.movePosition(QTextCursor::StartOfBlock);
                c.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                qDebug() << "should display directed message, erasing last rx activity line..." << c.selectedText();
                c.deletePreviousChar();
                c.deletePreviousChar();
                c.deleteChar();
                c.deleteChar();
            }

            // log it to the display!
            displayTextForFreq(ad.text, ad.freq, ad.utcTimestamp, false, true, false);

            // and send it to the network in case we want to interact with it from an external app...
            sendNetworkMessage("RX.DIRECTED", ad.text, {
                {"FROM", QVariant(d.from)},
                {"TO", QVariant(d.to)},
                {"CMD", QVariant(d.cmd)},
                {"GRID", QVariant(d.grid)},
                {"EXTRA", QVariant(d.extra)},
                {"TEXT", QVariant(d.text)},
                {"FREQ", QVariant(ad.freq)},
                {"SNR", QVariant(ad.snr)},
                {"UTC", QVariant(ad.utcTimestamp.toMSecsSinceEpoch())}
            });
        }

        // and mark the offset as a directed offset so future free text is displayed
        markOffsetDirected(ad.freq, isAllCall);

        // construct a reply, if needed
        QString reply;

        // QUERIED SNR
        if (d.cmd == "?") {
            reply = QString("%1 SNR %2").arg(d.from).arg(Varicode::formatSNR(d.snr));
        }
        // QUERIED QTH
        else if (d.cmd == "@" && !isAllCall) {
            QString qth = m_config.my_qth();
            if (qth.isEmpty()) {
                continue;
            }

            reply = QString("%1 QTH %2").arg(d.from).arg(qth);
        }
        // QUERIED GRID
        else if (d.cmd == "^" && !isAllCall) {
            QString grid = m_config.my_grid();
            if (grid.isEmpty()) {
                continue;
            }

            reply = QString("%1 GRID %2").arg(d.from).arg(grid);
        }
        // QUERIED STATION MESSAGE
        else if (d.cmd == "&" && !isAllCall) {
            reply = QString("%1 QTC %2").arg(d.from).arg(m_config.my_station());
        }
        // QUERIED STATIONS HEARD
        else if (d.cmd == "$" && !isAllCall) {
            int i = 0;
            int maxStations = 4;
            auto calls = m_callActivity.keys();
            qSort(calls.begin(), calls.end(), [this](QString
                const & a, QString
                const & b) {
                auto left = m_callActivity[a];
                auto right = m_callActivity[b];
                return right.snr < left.snr;
            });

            QStringList lines;

            foreach(auto call, calls) {
                if (i >= maxStations) {
                    break;
                }

                auto d = m_callActivity[call];

                lines.append(QString("<%1 SNR %2>").arg(d.call).arg(Varicode::formatSNR(d.snr)));

                i++;
            }

            lines.prepend(QString("<%1 HEARING>").arg(m_config.my_callsign()));
            reply = lines.join('\n');
        }
        // PROCESS RETRANSMIT
        else if (d.cmd == "|" && !isAllCall) {
            // TODO: jsherer - perhaps parse d.text and ensure it is a valid message as well as prefix it with our call...
            reply = QString("%1 ACK\n%2 DE %1").arg(d.from).arg(d.text);
        }
        // PROCESS BUFFERED MESSAGE
        else if (d.cmd == "#" && !isAllCall) {

            // open file /save/messages/[callsign].txt and append a message log entry...
            QFile f(QDir::toNativeSeparators(m_config.writeable_data_dir ().absolutePath()) + QString("/save/messages/%1.txt").arg(Radio::base_callsign(d.from)));
            if (f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
              QTextStream out(&f);
              auto df = dialFrequency();
              auto text = QString("%1\t%2MHz\t%3Hz\t%4dB\t%5");
              text = text.arg(d.utcTimestamp.toString());
              text = text.arg(Radio::frequency_MHz_string(df));
              text = text.arg(d.freq);
              text = text.arg(Varicode::formatSNR(d.snr));
              text = text.arg(d.text);
              out << text << endl;
              f.close();
            }

            reply = QString("%1 ACK").arg(d.from);
        }
        // PROCESS AGN
        else if (d.cmd == " AGN?" && !isAllCall && !m_lastTxMessage.isEmpty()) {
            reply = m_lastTxMessage;
        }
        // PROCESS BUFFERED QSO QUERY
        else if (d.cmd == " QSO"){
            auto who = d.text;
            if(who.isEmpty()){
                continue;
            }

            auto callsigns = Varicode::parseCallsigns(who);
            if(callsigns.isEmpty()){
                continue;
            }

            QStringList replies;
            auto baseCall = callsigns.first();
            foreach(auto cd, m_callActivity.values()){
                if(baseCall == cd.call || baseCall == Radio::base_callsign(cd.call)){
                    auto r = QString("%1 ACK %2 %3 (%4)").arg(d.from).arg(cd.call).arg(Varicode::formatSNR(cd.snr)).arg(since(cd.utcTimestamp));
                    replies.append(r);
                }
            }
            reply = replies.join("\n");
        }
        // PROCESS BUFFERED QTH
        else if (d.cmd == " GRID"){
           // 1. parse grids
           // 2. log it to reporting networks
           auto grids = Varicode::parseGrids(d.text);
           foreach(auto grid, grids){
               CallDetail cd = {};
               cd.bits = d.bits;
               cd.call = d.from;
               cd.freq = d.freq;
               cd.grid = grid;
               cd.snr = d.snr;
               cd.utcTimestamp = d.utcTimestamp;
               logCallActivity(cd, true);
           }

           continue;
        }
        // PROCESS APRS
        else if(d.cmd == " APRS:" && m_config.spot_to_reporting_networks()){
            m_aprsClient->enqueueThirdParty(Radio::base_callsign(d.from), d.text);
            reply = QString("%1 ACK").arg(d.from);
        }
        // PROCESS ALERT
        else if (d.cmd == "!" && !isAllCall) {
            QMessageBox * msgBox = new QMessageBox(this);
            msgBox->setIcon(QMessageBox::Information);

            auto header = QString("Message from %3 at %1 (%2):");
            header = header.arg(d.utcTimestamp.time().toString());
            header = header.arg(d.freq);
            header = header.arg(d.from);
            msgBox->setText(header);
            msgBox->setInformativeText(d.text);

            auto ab = msgBox->addButton("ACK", QMessageBox::AcceptRole);
            auto db = msgBox->addButton("Discard", QMessageBox::NoRole);

            connect(msgBox, & QMessageBox::buttonClicked, this, [this, d, db, ab](QAbstractButton * btn) {
                if (btn != ab) {
                    return;
                }

                enqueueMessage(PriorityHigh, QString("%1 ACK").arg(d.from), d.freq, nullptr);
            });

            msgBox->show();

            // make sure this is explicit
            continue;
        }

        if (reply.isEmpty()) {
            continue;
        }

        // do not queue ALLCALL replies if auto-reply is not checked
        if(!ui->autoReplyButton->isChecked() && isAllCall){
            continue;
        }

        // queue the reply here to be sent when a free interval is available on the frequency that was sent
        // unless, this is an allcall, to which we should be responding on a clear frequency offset
        // we always want to make sure that the directed cache has been updated at this point so we have the
        // most information available to make a frequency selection.
        enqueueMessage(PriorityNormal, reply, isAllCall ? -1 : d.freq, nullptr);
    }
}

void MainWindow::processSpots() {
    if(!ui->spotButton->isChecked()){
        m_rxCallQueue.clear();
        return;
    }

    if(m_rxCallQueue.isEmpty()){
        return;
    }

    // Is it ok to post spots to PSKReporter?
    int nsec = QDateTime::currentMSecsSinceEpoch() / 1000 - m_secBandChanged;
    bool okToPost = (nsec > (4 * m_TRperiod) / 5);
    if (!okToPost) {
        return;
    }

    // Process spots to be sent...
    pskSetLocal();
    aprsSetLocal();

    while(!m_rxCallQueue.isEmpty()){
        CallDetail d = m_rxCallQueue.dequeue();
        if(d.call.isEmpty()){
            continue;
        }
        qDebug() << "spotting call to reporting networks" << d.call << d.snr << d.freq;
        pskLogReport("FT8CALL", d.freq, d.snr, d.call, d.grid);
        aprsLogReport(d.freq, d.snr, d.call, d.grid);
    }
}

void MainWindow::processTxQueue(){
    if(m_txMessageQueue.isEmpty()){
        return;
    }

    // grab the next message...
    auto head = m_txMessageQueue.head();

    // decide if it's ok to transmit...
    int f = head.freq;
    if(f == -1){
        f = currentFreqOffset();
    }

    if(!isFreqOffsetFree(f, 60)){
        f = findFreeFreqOffset(500, 2000, 60);
    }

    // we need a valid frequency...
    if(f == 0){
        return;
    }

    // tx frame queue needs to be empty...
    if(!m_txFrameQueue.isEmpty()){
        return;
    }

    // our message box needs to be empty...
    if(!ui->extFreeTextMsgEdit->toPlainText().isEmpty()){
        return;
    }

    // and if we are a low priority message, we need to have not transmitted in the past 30 seconds...
    if(head.priority <= PriorityLow && m_lastTxTime.secsTo(QDateTime::currentDateTimeUtc()) <= 30){
        return;
    }

    // if so... dequeue the next message from the queue...
    auto message = m_txMessageQueue.dequeue();

    // add the message to the outgoing message text box
    addMessageText(message.message, true);

    // check to see if we have autoreply enabled...(or if this is a beacon and the beacon button is enabled)
    if(message.priority >= PriorityHigh   ||
       (ui->autoReplyButton->isChecked()) ||
       (ui->beaconButton->isChecked() && message.message.contains("BEACON"))
    ){
        // then try to set the frequency...
        setFreqOffsetForRestore(f, true);

        // then prepare to transmit...
        toggleTx(true);
    }

    if(message.callback){
        message.callback();
    }
}

void MainWindow::displayActivity(bool force) {
    if (!m_rxDisplayDirty && !force) {
        return;
    }

    // Band Activity
    displayBandActivity();

    // Call Activity
    displayCallActivity();

    m_rxDisplayDirty = false;
}

template<typename T>
QList<T> listCopyReverse(QList<T> const &list){
    QList<T> newList = QList<T>();
    auto iter = list.end();
    while(iter != list.begin()){
        newList.append(*(--iter));
    }
    return newList;
}

void MainWindow::displayBandActivity() {
    auto now = QDateTime::currentDateTimeUtc();

    // Selected Offset
    int selectedOffset = -1;
    auto selectedItems = ui->tableWidgetRXAll->selectedItems();
    if (!selectedItems.isEmpty()) {
        selectedOffset = selectedItems.first()->text().toInt();
    }

    ui->tableWidgetRXAll->setUpdatesEnabled(false);
    {
        // Scroll Position
        auto currentScrollPos = ui->tableWidgetRXAll->verticalScrollBar()->value();

        // Clear the table
        clearTableWidget(ui->tableWidgetRXAll);

        // Sort!
        QList < int > keys = m_bandActivity.keys();

        auto compareTimestamp = [this](const int left, int right) {
            auto leftItems = m_bandActivity[left];
            auto rightItems = m_bandActivity[right];

            if(leftItems.isEmpty()){
                return false;
            }

            if(rightItems.isEmpty()){
                return true;
            }

            auto leftLast = leftItems.last();
            auto rightLast = rightItems.last();

            return leftLast.utcTimestamp < rightLast.utcTimestamp;
        };

        auto compareSNR = [this](const int left, int right) {
            auto leftItems = m_bandActivity[left];
            auto rightItems = m_bandActivity[right];

            if(leftItems.isEmpty()){
                return false;
            }

            if(rightItems.isEmpty()){
                return true;
            }

            auto leftLast = leftItems.last();
            auto rightLast = rightItems.last();

            return leftLast.snr < rightLast.snr;
        };

        auto sortBy = getSortBy("bandActivity", "offset");
        bool reverse = false;
        if(sortBy.startsWith("-")){
            sortBy = sortBy.mid(1);
            reverse = true;
        }

        if(sortBy == "timestamp"){
            qSort(keys.begin(), keys.end(), compareTimestamp);
        } else if(sortBy == "snr"){
            qSort(keys.begin(), keys.end(), compareSNR);
        } else {
            // compare offset
            qSort(keys.begin(), keys.end());
        }

        if(reverse){
            keys = listCopyReverse(keys);
        }

        // Build the table
        foreach(int offset, keys) {
            QList < ActivityDetail > items = m_bandActivity[offset];
            if (items.length() > 0) {
                QStringList text;
                QString age;
                int snr = 0;
                int activityAging = m_config.activity_aging();
                foreach(ActivityDetail item, items) {
                    if (activityAging && item.utcTimestamp.secsTo(now) / 60 >= activityAging) {
                        continue;
                    }
                    if (item.text.isEmpty()) {
                        continue;
                    }
    #if 0
                    if (item.isCompound || (item.isDirected && item.text.contains("<....>"))){
                        //continue;
                        item.text = "[...]";
                    }
    #endif
                    if (item.isLowConfidence) {
                        item.text = QString("[%1]").arg(item.text);
                    }
                    if ((item.bits & Varicode::FT8CallLast) == Varicode::FT8CallLast) {
                        // can also use \u0004 \u2666 \u2404
                        item.text = QString("%1 \u2301 ").arg(item.text);
                    }
                    text.append(item.text);
                    snr = item.snr;
                    age = since(item.utcTimestamp);
                }

                auto joined = text.join("");
                if (joined.isEmpty()) {
                    continue;
                }

                ui->tableWidgetRXAll->insertRow(ui->tableWidgetRXAll->rowCount());
                int row = ui->tableWidgetRXAll->rowCount() - 1;

                auto offsetItem = new QTableWidgetItem(QString("%1").arg(offset));
                offsetItem->setData(Qt::UserRole, QVariant(offset));
                ui->tableWidgetRXAll->setItem(row, 0, offsetItem);

                auto ageItem = new QTableWidgetItem(QString("(%1)").arg(age));
                ageItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
                ui->tableWidgetRXAll->setItem(row, 1, ageItem);

                auto snrItem = new QTableWidgetItem(QString("%1").arg(Varicode::formatSNR(snr)));
                snrItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
                ui->tableWidgetRXAll->setItem(row, 2, snrItem);

                // align right if eliding...
                int colWidth = ui->tableWidgetRXAll->columnWidth(3);
                auto textItem = new QTableWidgetItem(joined);
                QFontMetrics fm(textItem->font());
                auto elidedText = fm.elidedText(joined, Qt::ElideLeft, colWidth);
                auto flag = Qt::AlignLeft | Qt::AlignVCenter;
                if (elidedText != joined) {
                    flag = Qt::AlignRight | Qt::AlignVCenter;
                    textItem->setText(joined);
                }
                textItem->setTextAlignment(flag);

                if (text.last().contains(QRegularExpression {
                        "\\b(CQCQCQ|BEACON)\\b"
                    })) {
                    offsetItem->setBackground(QBrush(m_config.color_CQ()));
                    ageItem->setBackground(QBrush(m_config.color_CQ()));
                    snrItem->setBackground(QBrush(m_config.color_CQ()));
                    textItem->setBackground(QBrush(m_config.color_CQ()));
                }

                bool isDirectedAllCall = false;
                if (isDirectedOffset(offset, &isDirectedAllCall) && !isDirectedAllCall) {
                    offsetItem->setBackground(QBrush(m_config.color_MyCall()));
                    ageItem->setBackground(QBrush(m_config.color_MyCall()));
                    snrItem->setBackground(QBrush(m_config.color_MyCall()));
                    textItem->setBackground(QBrush(m_config.color_MyCall()));
                }

                ui->tableWidgetRXAll->setItem(row, 3, textItem);

                if (offset == selectedOffset) {
                    for(int i = 0; i < ui->tableWidgetRXAll->columnCount(); i++){
                        ui->tableWidgetRXAll->item(row, i)->setSelected(true);
                    }
                }
            }
        }

        // Resize the table columns
        ui->tableWidgetRXAll->resizeColumnToContents(0);
        ui->tableWidgetRXAll->resizeColumnToContents(1);
        ui->tableWidgetRXAll->resizeColumnToContents(2);

        // Reset the scroll position
        ui->tableWidgetRXAll->verticalScrollBar()->setValue(currentScrollPos);
    }
    ui->tableWidgetRXAll->setUpdatesEnabled(true);
}

void MainWindow::displayCallActivity() {
    auto now = QDateTime::currentDateTimeUtc();

    // Selected callsign
    QString selectedCall = callsignSelected();

    auto currentScrollPos = ui->tableWidgetCalls->verticalScrollBar()->value();

    ui->tableWidgetCalls->setUpdatesEnabled(false);
    {
        // Clear the table
        clearTableWidget(ui->tableWidgetCalls);

        // Create the ALLCALL item
        auto item = new QTableWidgetItem("ALLCALL");
        ui->tableWidgetCalls->insertRow(ui->tableWidgetCalls->rowCount());
        item->setData(Qt::UserRole, QVariant("ALLCALL"));
        ui->tableWidgetCalls->setItem(ui->tableWidgetCalls->rowCount() - 1, 0, item);
        ui->tableWidgetCalls->setSpan(ui->tableWidgetCalls->rowCount() - 1, 0, 1, ui->tableWidgetCalls->columnCount());
        if (isAllCallIncluded(selectedCall)) {
            ui->tableWidgetCalls->selectRow(ui->tableWidgetCalls->rowCount() - 1);
        }

        // Build the table
        QList < QString > keys = m_callActivity.keys();

        auto compareOffset = [this](const QString left, QString right) {
            auto leftActivity = m_callActivity[left];
            auto rightActivity = m_callActivity[right];

            return leftActivity.freq < rightActivity.freq;
        };

        auto compareDistance = [this](const QString left, QString right) {
            auto leftActivity = m_callActivity[left];
            auto rightActivity = m_callActivity[right];

            if(leftActivity.grid.isEmpty()){
                return false;
            }

            if(rightActivity.grid.isEmpty()){
                return true;
            }

            int leftDistance = 0;
            int rightDistance = 0;
            calculateDistance(leftActivity.grid, &leftDistance);
            calculateDistance(rightActivity.grid, &rightDistance);

            return leftDistance < rightDistance;
        };

        auto compareTimestamp = [this](const QString left, QString right) {
            auto leftActivity = m_callActivity[left];
            auto rightActivity = m_callActivity[right];

            return leftActivity.utcTimestamp < rightActivity.utcTimestamp;
        };

        auto compareSNR = [this](const QString left, QString right) {
            auto leftActivity = m_callActivity[left];
            auto rightActivity = m_callActivity[right];

            return leftActivity.snr < rightActivity.snr;
        };

        auto sortBy = getSortBy("callActivity", "callsign");
        bool reverse = false;
        if(sortBy.startsWith("-")){
            sortBy = sortBy.mid(1);
            reverse = true;
        }

        if(sortBy == "offset"){
            qSort(keys.begin(), keys.end(), compareOffset);
        } else if(sortBy == "distance"){
            qSort(keys.begin(), keys.end(), compareDistance);
        } else if(sortBy == "timestamp"){
            qSort(keys.begin(), keys.end(), compareTimestamp);
        } else if(sortBy == "snr"){
            qSort(keys.begin(), keys.end(), compareSNR);
        } else {
            // compare callsign
            qSort(keys.begin(), keys.end());
        }

        if(reverse){
            keys = listCopyReverse(keys);
        }

        int callsignAging = m_config.callsign_aging();
        foreach(QString call, keys) {
            CallDetail d = m_callActivity[call];

            if (callsignAging && d.utcTimestamp.secsTo(now) / 60 >= callsignAging) {
                continue;
            }

            ui->tableWidgetCalls->insertRow(ui->tableWidgetCalls->rowCount());
            int row = ui->tableWidgetCalls->rowCount() - 1;

            QString displayCall = d.through.isEmpty() ? d.call : QString("%1 | %2").arg(d.through).arg(d.call);
            auto displayItem = new QTableWidgetItem(displayCall);
            displayItem->setData(Qt::UserRole, QVariant((d.call)));
            ui->tableWidgetCalls->setItem(row, 0, displayItem);
            ui->tableWidgetCalls->setItem(row, 1, new QTableWidgetItem(QString("(%1)").arg(since(d.utcTimestamp))));
            ui->tableWidgetCalls->setItem(row, 2, new QTableWidgetItem(QString("%1").arg(d.freq)));
            ui->tableWidgetCalls->setItem(row, 3, new QTableWidgetItem(QString("%1").arg(Varicode::formatSNR(d.snr))));
            ui->tableWidgetCalls->setItem(row, 4, new QTableWidgetItem(QString("%1").arg(d.grid)));

            auto distanceItem = new QTableWidgetItem(calculateDistance(d.grid));
            distanceItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            ui->tableWidgetCalls->setItem(ui->tableWidgetCalls->rowCount() - 1, 5, distanceItem);

            if (call == selectedCall) {
                for(int i = 0; i < ui->tableWidgetCalls->columnCount(); i++){
                    ui->tableWidgetCalls->item(row, i)->setSelected(true);
                }
            }
        }

        // Resize the table columns
        ui->tableWidgetCalls->resizeColumnToContents(0);
        ui->tableWidgetCalls->resizeColumnToContents(1);
        ui->tableWidgetCalls->resizeColumnToContents(2);
        ui->tableWidgetCalls->resizeColumnToContents(3);
        ui->tableWidgetCalls->resizeColumnToContents(4);

        // Reset the scroll position
        ui->tableWidgetCalls->verticalScrollBar()->setValue(currentScrollPos);
    }
    ui->tableWidgetCalls->setUpdatesEnabled(true);
}

void MainWindow::postWSPRDecode (bool is_new, QStringList parts)
{
#if 0
    if (parts.size () < 8)
    {
      parts.insert (6, "");
    }

  m_messageClient->WSPR_decode (is_new, QTime::fromString (parts[0], "hhmm"), parts[1].toInt ()
                                , parts[2].toFloat (), Radio::frequency (parts[3].toFloat (), 6)
                                , parts[4].toInt (), parts[5], parts[6], parts[7].toInt ()
                                , m_diskData);
#endif
}

void MainWindow::networkMessage(Message const &message)
{
    if(!m_config.accept_udp_requests()){
        return;
    }

    auto type = message.type();

    if(type == "PONG"){
        return;
    }

    // Inspired by FLDigi
    // TODO: MAIN.RX - Turn on RX
    // TODO: MAIN.TX - Transmit
    // TODO: MAIN.TUNE - Tune
    // TODO: MAIN.HALT - Halt

    // RIG.GET_FREQ - Get the current Frequency
    // RIG.SET_FREQ - Set the current Frequency
    if(type == "RIG.GET_FREQ"){
        sendNetworkMessage("RIG.FREQ", "", {
            {"DIAL", QVariant((quint64)dialFrequency())},
            {"OFFSET", QVariant((quint64)currentFreqOffset())}
        });
        return;
    }

    if(type == "RIG.SET_FREQ"){
        auto params = message.params();
        if(params.contains("DIAL")){
            bool ok = false;
            auto f = params["DIAL"].toInt(&ok);
            if(ok){
                setRig(f);
                displayDialFrequency();
            }
        }
        if(params.contains("OFFSET")){
            bool ok = false;
            auto f = params["OFFSET"].toInt(&ok);
            if(ok){
                setFreqOffsetForRestore(f, false);
            }
        }
    }

    // STATION.GET_CALLSIGN - Get the current callsign
    // STATION.GET_GRID - Get the current grid locator
    // STATION.SET_GRID - Set the current grid locator
    // STATION.GET_QTC - Get the current station message
    // STATION.SET_QTC - Set the current station message
    if(type == "STATION.GET_CALLSIGN"){
        sendNetworkMessage("STATION.CALLSIGN", m_config.my_callsign());
        return;
    }

    if(type == "STATION.GET_GRID"){
        sendNetworkMessage("STATION.GRID", m_config.my_grid());
        return;
    }

    if(type == "STATION.SET_GRID"){
        m_config.set_dynamic_location(message.value());
        sendNetworkMessage("STATION.GRID", m_config.my_grid());
        return;
    }

    if(type == "STATION.GET_QTC"){
        sendNetworkMessage("STATION.QTC", m_config.my_station());
        return;
    }

    if(type == "STATION.SET_QTC"){
        m_config.set_dynamic_station_message(message.value());
        sendNetworkMessage("STATION.QTC", m_config.my_station());
        return;
    }

    // RX.GET_CALL_ACTIVITY
    // RX.GET_CALL_SELECTED
    // RX.GET_BAND_ACTIVITY
    // RX.GET_TEXT

    if(type == "RX.GET_CALL_ACTIVITY"){
        QMap<QString, QVariant> calls;
        foreach(auto cd, m_callActivity.values()){
            QMap<QString, QVariant> detail;
            detail["SNR"] = QVariant(cd.snr);
            detail["GRID"] = QVariant(cd.grid);
            detail["UTC"] = QVariant(cd.utcTimestamp.toMSecsSinceEpoch());
            calls[cd.call] = QVariant(detail);
        }

        sendNetworkMessage("RX.CALL_ACTIVITY", "", calls);
        return;
    }

    if(type == "RX.GET_CALL_SELECTED"){
        sendNetworkMessage("RX.CALL_SELECTED", callsignSelected());
        return;
    }

    if(type == "RX.GET_BAND_ACTIVITY"){
        QMap<QString, QVariant> offsets;
        foreach(auto offset, m_bandActivity.keys()){
            auto activity = m_bandActivity[offset];
            if(activity.isEmpty()){
                continue;
            }

            auto d = activity.last();

            QMap<QString, QVariant> detail;
            detail["FREQ"] = QVariant(d.freq);
            detail["TEXT"] = QVariant(d.text);
            detail["SNR"] = QVariant(d.snr);
            detail["UTC"] = QVariant(d.utcTimestamp.toMSecsSinceEpoch());
            offsets[QString("%1").arg(offset)] = QVariant(detail);
        }

        sendNetworkMessage("RX.BAND_ACTIVITY", "", offsets);
        return;
    }

    if(type == "RX.GET_TEXT"){
        sendNetworkMessage("RX.TEXT", ui->textEditRX->toPlainText());
        return;
    }

    // TX.GET_TEXT
    // TX.SET_TEXT
    // TX.SEND_MESSAGE

    if(type == "TX.GET_TEXT"){
        sendNetworkMessage("TX.TEXT", ui->extFreeTextMsgEdit->toPlainText());
        return;
    }

    if(type == "TX.SET_TEXT"){
        addMessageText(message.value(), true);
        return;
    }

    if(type == "TX.SEND_MESSAGE"){
        auto text = message.value();
        if(!text.isEmpty()){
            enqueueMessage(PriorityNormal, text, -1, nullptr);
            return;
        }
    }

    // WINDOW.RAISE

    if(type == "WINDOW.RAISE"){
        setWindowState(Qt::WindowActive);
        activateWindow();
        return;
    }

    qDebug() << "Unable to process networkMessage:" << type;
}

void MainWindow::sendNetworkMessage(QString const &type, QString const &message){
    m_messageClient->send(Message(type, message));
}

void MainWindow::sendNetworkMessage(QString const &type, QString const &message, QMap<QString, QVariant> const &params)
{
    m_messageClient->send(Message(type, message, params));
}

void MainWindow::networkError (QString const& e)
{
  if(!m_config.accept_udp_requests()){
    return;
  }
  if (m_splash && m_splash->isVisible ()) m_splash->hide ();
  if (MessageBox::Retry == MessageBox::warning_message (this, tr ("Network Error")
                                                        , tr ("Error: %1\nUDP server %2:%3")
                                                        .arg (e)
                                                        .arg (m_config.udp_server_name ())
                                                        .arg (m_config.udp_server_port ())
                                                        , QString {}
                                                        , MessageBox::Cancel | MessageBox::Retry
                                                        , MessageBox::Cancel))
    {
      // retry server lookup
      m_messageClient->set_server (m_config.udp_server_name ());
    }
}

void MainWindow::on_syncSpinBox_valueChanged(int n)
{
  m_minSync=n;
}

void MainWindow::p1ReadFromStdout()                        //p1readFromStdout
{
  QString t1;
  while(p1.canReadLine()) {
    QString t(p1.readLine());
    if(t.indexOf("<DecodeFinished>") >= 0) {
      m_bDecoded = m_nWSPRdecodes > 0;
      if(!m_diskData) {
        WSPR_history(m_dialFreqRxWSPR, m_nWSPRdecodes);
        if(m_nWSPRdecodes==0 and ui->band_hopping_group_box->isChecked()) {
          t = " Receiving " + m_mode + " ----------------------- " +
              m_config.bands ()->find (m_dialFreqRxWSPR);
          t=WSPR_hhmm(-60) + ' ' + t.rightJustified (66, '-');
          ui->decodedTextBrowser->appendText(t);
        }
        killFileTimer.start (45*1000); //Kill in 45s (for slow modes)
      }
      m_nWSPRdecodes=0;
      ui->DecodeButton->setChecked (false);
      if(m_uploadSpots
         && m_config.is_transceiver_online ()) { // need working rig control
        float x=qrand()/((double)RAND_MAX + 1.0);
        int msdelay=20000*x;
        uploadTimer.start(msdelay);                         //Upload delay
      } else {
        QFile f(QDir::toNativeSeparators(m_config.writeable_data_dir ().absolutePath()) + "/wspr_spots.txt");
        if(f.exists()) f.remove();
      }
      m_RxLog=0;
      m_startAnother=m_loopall;
      m_blankLine=true;
      m_decoderBusy = false;
      statusUpdate ();
    } else {

      int n=t.length();
      t=t.mid(0,n-2) + "                                                  ";
      t.remove(QRegExp("\\s+$"));
      QStringList rxFields = t.split(QRegExp("\\s+"));
      QString rxLine;
      QString grid="";
      if ( rxFields.count() == 8 ) {
          rxLine = QString("%1 %2 %3 %4 %5   %6  %7  %8")
                  .arg(rxFields.at(0), 4)
                  .arg(rxFields.at(1), 4)
                  .arg(rxFields.at(2), 5)
                  .arg(rxFields.at(3), 11)
                  .arg(rxFields.at(4), 4)
                  .arg(rxFields.at(5).leftJustified (12))
                  .arg(rxFields.at(6), -6)
                  .arg(rxFields.at(7), 3);
          postWSPRDecode (true, rxFields);
          grid = rxFields.at(6);
      } else if ( rxFields.count() == 7 ) { // Type 2 message
          rxLine = QString("%1 %2 %3 %4 %5   %6  %7  %8")
                  .arg(rxFields.at(0), 4)
                  .arg(rxFields.at(1), 4)
                  .arg(rxFields.at(2), 5)
                  .arg(rxFields.at(3), 11)
                  .arg(rxFields.at(4), 4)
                  .arg(rxFields.at(5).leftJustified (12))
                  .arg("", -6)
                  .arg(rxFields.at(6), 3);
          postWSPRDecode (true, rxFields);
      } else {
          rxLine = t;
      }
      if(grid!="") {
        double utch=0.0;
        int nAz,nEl,nDmiles,nDkm,nHotAz,nHotABetter;
        azdist_(const_cast <char *> (m_config.my_grid ().toLatin1().constData()),
                const_cast <char *> (grid.toLatin1().constData()),&utch,
                &nAz,&nEl,&nDmiles,&nDkm,&nHotAz,&nHotABetter,6,6);
        QString t1;
        if(m_config.miles()) {
          t1.sprintf("%7d",nDmiles);
        } else {
          t1.sprintf("%7d",nDkm);
        }
        rxLine += t1;
      }

      if (m_config.insert_blank () && m_blankLine) {
        QString band;
        Frequency f=1000000.0*rxFields.at(3).toDouble()+0.5;
        band = ' ' + m_config.bands ()->find (f);
        ui->decodedTextBrowser->appendText(band.rightJustified (71, '-'));
        m_blankLine = false;
      }
      m_nWSPRdecodes += 1;
      ui->decodedTextBrowser->appendText(rxLine);
    }
  }
}

QString MainWindow::WSPR_hhmm(int n)
{
  QDateTime t=QDateTime::currentDateTimeUtc().addSecs(n);
  int m=t.toString("hhmm").toInt()/2;
  QString t1;
  t1.sprintf("%04d",2*m);
  return t1;
}

void MainWindow::WSPR_history(Frequency dialFreq, int ndecodes)
{
  QDateTime t=QDateTime::currentDateTimeUtc().addSecs(-60);
  QString t1=t.toString("yyMMdd");
  QString t2=WSPR_hhmm(-60);
  QString t3;
  t3.sprintf("%13.6f",0.000001*dialFreq);
  if(ndecodes<0) {
    t1=t1 + " " + t2 + t3 + "  T";
  } else {
    QString t4;
    t4.sprintf("%4d",ndecodes);
    t1=t1 + " " + t2 + t3 + "  R" + t4;
  }
  QFile f {m_config.writeable_data_dir ().absoluteFilePath ("WSPR_history.txt")};
  if (f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
    QTextStream out(&f);
    out << t1 << endl;
    f.close();
  } else {
    MessageBox::warning_message (this, tr ("File Error")
                                 , tr ("Cannot open \"%1\" for append: %2")
                                 .arg (f.fileName ()).arg (f.errorString ()));
  }
}


void MainWindow::uploadSpots()
{
  // do not spot replays or if rig control not working
  if(m_diskData || !m_config.is_transceiver_online ()) return;
  if(m_uploading) {
    qDebug() << "Previous upload has not completed, spots were lost";
    wsprNet->abortOutstandingRequests ();
    m_uploading = false;
  }
  QString rfreq = QString("%1").arg(0.000001*(m_dialFreqRxWSPR + 1500), 0, 'f', 6);
  QString tfreq = QString("%1").arg(0.000001*(m_dialFreqRxWSPR +
                        ui->TxFreqSpinBox->value()), 0, 'f', 6);
  wsprNet->upload(m_config.my_callsign(), m_config.my_grid(), rfreq, tfreq,
                  m_mode, QString::number(ui->autoButton->isChecked() ? m_pctx : 0),
                  QString::number(m_dBm), version(),
                  QDir::toNativeSeparators(m_config.writeable_data_dir ().absolutePath()) + "/wspr_spots.txt");
  m_uploading = true;
}

void MainWindow::uploadResponse(QString response)
{
  if (response == "done") {
    m_uploading=false;
  } else {
    if (response.startsWith ("Upload Failed")) {
      m_uploading=false;
    }
    qDebug () << "WSPRnet.org status:" << response;
  }
}

void MainWindow::on_TxPowerComboBox_currentIndexChanged(const QString &arg1)
{
  int i1=arg1.indexOf(" ");
  m_dBm=arg1.mid(0,i1).toInt();
}

void MainWindow::on_sbTxPercent_valueChanged(int n)
{
  m_pctx=n;
  if(m_pctx>0) {
    ui->pbTxNext->setEnabled(true);
  } else {
    m_txNext=false;
    ui->pbTxNext->setChecked(false);
    ui->pbTxNext->setEnabled(false);
  }
}

void MainWindow::on_cbUploadWSPR_Spots_toggled(bool b)
{
  m_uploadSpots=b;
  if(m_uploadSpots) ui->cbUploadWSPR_Spots->setStyleSheet("");
  if(!m_uploadSpots) ui->cbUploadWSPR_Spots->setStyleSheet(
        "QCheckBox{background-color: yellow}");
}

void MainWindow::on_WSPRfreqSpinBox_valueChanged(int n)
{
  ui->TxFreqSpinBox->setValue(n);
}

void MainWindow::on_pbTxNext_clicked(bool b)
{
  m_txNext=b;
}

void MainWindow::WSPR_scheduling ()
{
  m_WSPR_tx_next = false;
  if (m_config.is_transceiver_online () // need working rig control for hopping
      && !m_config.is_dummy_rig ()
      && ui->band_hopping_group_box->isChecked ()) {
    auto hop_data = m_WSPR_band_hopping.next_hop (m_auto);
    qDebug () << "hop data: period:" << hop_data.period_name_
              << "frequencies index:" << hop_data.frequencies_index_
              << "tune:" << hop_data.tune_required_
              << "tx:" << hop_data.tx_next_;
    m_WSPR_tx_next = hop_data.tx_next_;
    if (hop_data.frequencies_index_ >= 0) { // new band
      ui->bandComboBox->setCurrentIndex (hop_data.frequencies_index_);
      on_bandComboBox_activated (hop_data.frequencies_index_);
      m_cmnd.clear ();
      QStringList prefixes {".bat", ".cmd", ".exe", ""};
      for (auto const& prefix : prefixes)
        {
          auto const& path = m_appDir + "/user_hardware" + prefix;
          QFile f {path};
          if (f.exists ()) {
            m_cmnd = QDir::toNativeSeparators (f.fileName ()) + ' ' +
              m_config.bands ()->find (m_freqNominal).remove ('m');
          }
        }
      if(m_cmnd!="") p3.start(m_cmnd);     // Execute user's hardware controller

      // Produce a short tuneup signal
      m_tuneup = false;
      if (hop_data.tune_required_) {
        m_tuneup = true;
        on_tuneButton_clicked (true);
        tuneATU_Timer.start (2500);
      }
    }

    // Display grayline status
    band_hopping_label.setText (hop_data.period_name_);
  }
  else {
    m_WSPR_tx_next = m_WSPR_band_hopping.next_is_tx ("WSPR-LF" == m_mode);
  }
}

void MainWindow::astroUpdate ()
{
  if (m_astroWidget)
    {
      // no Doppler correction while CTRL pressed allows manual tuning
      if (Qt::ControlModifier & QApplication::queryKeyboardModifiers ()) return;

      auto correction = m_astroWidget->astroUpdate(QDateTime::currentDateTimeUtc (),
                                                   m_config.my_grid(), m_hisGrid,
                                                   m_freqNominal,
                                                   "Echo" == m_mode, m_transmitting,
                                                   !m_config.tx_QSY_allowed (), m_TRperiod);
      // no Doppler correction in Tx if rig can't do it
      if (m_transmitting && !m_config.tx_QSY_allowed ()) return;
      if (!m_astroWidget->doppler_tracking ()) return;
      if ((m_monitoring || m_transmitting)
          // no Doppler correction below 6m
          && m_freqNominal >= 50000000
          && m_config.split_mode ())
        {
          // adjust for rig resolution
          if (m_config.transceiver_resolution () > 2)
            {
              correction.rx = (correction.rx + 50) / 100 * 100;
              correction.tx = (correction.tx + 50) / 100 * 100;
            }
          else if (m_config.transceiver_resolution () > 1)
            {
              correction.rx = (correction.rx + 10) / 20 * 20;
              correction.tx = (correction.tx + 10) / 20 * 20;
            }
          else if (m_config.transceiver_resolution () > 0)
            {
              correction.rx = (correction.rx + 5) / 10 * 10;
              correction.tx = (correction.tx + 5) / 10 * 10;
            }
          else if (m_config.transceiver_resolution () < -2)
            {
              correction.rx = correction.rx / 100 * 100;
              correction.tx = correction.tx / 100 * 100;
            }
          else if (m_config.transceiver_resolution () < -1)
            {
              correction.rx = correction.rx / 20 * 20;
              correction.tx = correction.tx / 20 * 20;
            }
          else if (m_config.transceiver_resolution () < 0)
            {
              correction.rx = correction.rx / 10 * 10;
              correction.tx = correction.tx / 10 * 10;
            }
          m_astroCorrection = correction;
        }
      else
        {
          m_astroCorrection = {};
        }

      setRig ();
    }
}

void MainWindow::setRig (Frequency f)
{
  if (f)
    {
      m_freqNominal = f;
      genCQMsg ();
      m_freqTxNominal = m_freqNominal;
      if (m_astroWidget) m_astroWidget->nominal_frequency (m_freqNominal, m_freqTxNominal);
    }
  if (m_mode == "FreqCal"
      && m_frequency_list_fcal_iter != m_config.frequencies ()->end ()) {
    m_freqNominal = m_frequency_list_fcal_iter->frequency_ - ui->RxFreqSpinBox->value ();
  }
  if(m_transmitting && !m_config.tx_QSY_allowed ()) return;
  if ((m_monitoring || m_transmitting) && m_config.transceiver_online ())
    {
      if (m_transmitting && m_config.split_mode ())
        {
          Q_EMIT m_config.transceiver_tx_frequency (m_freqTxNominal + m_astroCorrection.tx);
        }
      else
        {
          Q_EMIT m_config.transceiver_frequency (m_freqNominal + m_astroCorrection.rx);
        }
    }
}

void MainWindow::fastPick(int x0, int x1, int y)
{
  float pixPerSecond=12000.0/512.0;
  if(m_TRperiod<30) pixPerSecond=12000.0/256.0;
  if(m_mode!="ISCAT" and m_mode!="MSK144") return;
  if(!m_decoderBusy) {
    dec_data.params.newdat=0;
    dec_data.params.nagain=1;
    m_blankLine=false;                 // don't insert the separator again
    m_nPick=1;
    if(y > 120) m_nPick=2;
    m_t0Pick=x0/pixPerSecond;
    m_t1Pick=x1/pixPerSecond;
    m_dataAvailable=true;
    decode();
  }
}

void MainWindow::on_actionMeasure_reference_spectrum_triggered()
{
  if(!m_monitoring) on_monitorButton_clicked (true);
  m_bRefSpec=true;
}

void MainWindow::on_actionMeasure_phase_response_triggered()
{
  if(m_bTrain) {
    m_bTrain=false;
    MessageBox::information_message (this, tr ("Phase Training Disabled"));
  } else {
    m_bTrain=true;
    MessageBox::information_message (this, tr ("Phase Training Enabled"));
  }
}

void MainWindow::on_actionErase_reference_spectrum_triggered()
{
  m_bClearRefSpec=true;
}

void MainWindow::freqCalStep()
{
  if (m_frequency_list_fcal_iter == m_config.frequencies ()->end ()
      || ++m_frequency_list_fcal_iter == m_config.frequencies ()->end ()) {
    m_frequency_list_fcal_iter = m_config.frequencies ()->begin ();
  }

  // allow for empty list
  if (m_frequency_list_fcal_iter != m_config.frequencies ()->end ()) {
    setRig (m_frequency_list_fcal_iter->frequency_ - ui->RxFreqSpinBox->value ());
  }
}

void MainWindow::on_sbCQTxFreq_valueChanged(int)
{
  setXIT (ui->TxFreqSpinBox->value ());
}

void MainWindow::on_cbCQTx_toggled(bool b)
{
  ui->sbCQTxFreq->setEnabled(b);
  genCQMsg();
  if(b) {
    ui->txrb6->setChecked(true);
    m_ntx=6;
    m_QSOProgress = CALLING;
  }
  setRig ();
  setXIT (ui->TxFreqSpinBox->value ());
}

void MainWindow::statusUpdate () const
{
#if 0
  if (!ui) return;
  auto submode = current_submode ();

  m_messageClient->status_update (m_freqNominal, m_mode, m_hisCall,
                                  QString::number (ui->rptSpinBox->value ()),
                                  m_modeTx, ui->autoButton->isChecked (),
                                  m_transmitting, m_decoderBusy,
                                  ui->RxFreqSpinBox->value (), ui->TxFreqSpinBox->value (),
                                  m_config.my_callsign (), m_config.my_grid (),
                                  m_hisGrid, m_tx_watchdog,
                                  submode != QChar::Null ? QString {submode} : QString {}, m_bFastMode);
#endif
}

void MainWindow::childEvent (QChildEvent * e)
{
  if (e->child ()->isWidgetType ())
    {
      switch (e->type ())
        {
        case QEvent::ChildAdded: add_child_to_event_filter (e->child ()); break;
        case QEvent::ChildRemoved: remove_child_from_event_filter (e->child ()); break;
        default: break;
        }
    }
  QMainWindow::childEvent (e);
}

// add widget and any child widgets to our event filter so that we can
// take action on key press ad mouse press events anywhere in the main window
void MainWindow::add_child_to_event_filter (QObject * target)
{
  if (target && target->isWidgetType ())
    {
      target->installEventFilter (this);
    }
  auto const& children = target->children ();
  for (auto iter = children.begin (); iter != children.end (); ++iter)
    {
      add_child_to_event_filter (*iter);
    }
}

// recursively remove widget and any child widgets from our event filter
void MainWindow::remove_child_from_event_filter (QObject * target)
{
  auto const& children = target->children ();
  for (auto iter = children.begin (); iter != children.end (); ++iter)
    {
      remove_child_from_event_filter (*iter);
    }
  if (target && target->isWidgetType ())
    {
      target->removeEventFilter (this);
    }
}

void MainWindow::tx_watchdog (bool triggered)
{
  auto prior = m_tx_watchdog;
  m_tx_watchdog = triggered;
  if (triggered)
    {
      m_bTxTime=false;
      if (m_tune) stop_tuning ();
      if (m_auto) auto_tx_mode (false);
      tx_status_label.setStyleSheet ("QLabel{background-color: #ff0000}");
      tx_status_label.setText ("Runaway Tx watchdog");
      QApplication::alert (this);
    }
  else
    {
      m_idleMinutes = 0;
      update_watchdog_label ();
    }
  if (prior != triggered) statusUpdate ();
}

void MainWindow::update_watchdog_label ()
{
  if (m_config.watchdog () && !m_mode.startsWith ("WSPR"))
    {
      watchdog_label.setText (QString {"WD:%1m"}.arg (m_config.watchdog () - m_idleMinutes));
      watchdog_label.setVisible (true);
    }
  else
    {
      watchdog_label.setText (QString {});
      watchdog_label.setVisible (false);
    }
}

void MainWindow::on_cbMenus_toggled(bool b)
{
  hideMenus(!b);
}

void MainWindow::on_cbCQonly_toggled(bool)
{
  QFile {m_config.temp_dir().absoluteFilePath(".lock")}.remove(); // Allow jt9 to start
  decodeBusy(true);
}

void MainWindow::on_cbFirst_toggled(bool b)
{
  if (b) {
    if (m_auto && CALLING == m_QSOProgress) {
      ui->cbFirst->setStyleSheet ("QCheckBox{color:red}");
    }
  } else {
    ui->cbFirst->setStyleSheet ("");
  }
}

void MainWindow::on_cbAutoSeq_toggled(bool b)
{
  if(!b) ui->cbFirst->setChecked(false);
  ui->cbFirst->setVisible((m_mode=="FT8") and b);
}

void MainWindow::on_measure_check_box_stateChanged (int state)
{
  m_config.enable_calibration (Qt::Checked != state);
}

void MainWindow::write_transmit_entry (QString const& file_name)
{
  QFile f {m_config.writeable_data_dir ().absoluteFilePath (file_name)};
  if (f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append))
    {
      QTextStream out(&f);
      auto time = QDateTime::currentDateTimeUtc ();
      time = time.addSecs (-(time.time ().second () % m_TRperiod));
      auto dt = DecodedText(m_currentMessage);
      out << time.toString("yyMMdd_hhmmss")
          << "  Transmitting " << qSetRealNumberPrecision (12) << (m_freqNominal / 1.e6)
          << " MHz  " << m_modeTx
          << ":  " << dt.message() << endl;
      f.close();
    }
  else
    {
      auto const& message = tr ("Cannot open \"%1\" for append: %2")
        .arg (f.fileName ()).arg (f.errorString ());
#if QT_VERSION >= 0x050400
      QTimer::singleShot (0, [=] {                   // don't block guiUpdate
          MessageBox::warning_message (this, tr ("Log File Error"), message);
        });
#else
      MessageBox::warning_message (this, tr ("Log File Error"), message);
#endif
    }
}

// -------------------------- Code for FT8 DXpedition Mode ---------------------------

void MainWindow::hound_reply ()
{
  if (!m_tune) {
    //Select TX3, set TxFreq to FoxFreq, and Force Auto ON.
    ui->txrb3->setChecked (true);
    m_nSentFoxRrpt = 1;
    ui->rptSpinBox->setValue(m_rptSent.toInt());
    if (!m_auto) auto_tx_mode(true);
    ui->TxFreqSpinBox->setValue (m_nFoxFreq);
  }
}

void MainWindow::on_sbNlist_valueChanged(int n)
{
  m_Nlist=n;
}

void MainWindow::on_sbNslots_valueChanged(int n)
{
  m_Nslots=n;
  QString t;
  t.sprintf(" NSlots %d",m_Nslots);
  writeFoxQSO(t);
}

void MainWindow::on_sbMax_dB_valueChanged(int n)
{
  m_max_dB=n;
  QString t;
  t.sprintf(" Max_dB %d",m_max_dB);
  writeFoxQSO(t);
}

void MainWindow::on_pbFoxReset_clicked()
{
  auto button = MessageBox::query_message (this, tr ("Confirm Reset"),
      tr ("Are you sure you want to clear the QSO queues?"));
  if(button == MessageBox::Yes) {
    QFile f(m_config.temp_dir().absoluteFilePath("houndcallers.txt"));
    f.remove();
    ui->decodedTextBrowser->setText("");
    ui->textBrowser4->setText("");
    m_houndQueue.clear();
    m_foxQSO.clear();
    m_foxQSOinProgress.clear();
    writeFoxQSO(" Reset");
  }
}

void MainWindow::on_comboBoxHoundSort_activated(int index)
{
  if(index!=-99) houndCallers();            //Silence compiler warning
}

//------------------------------------------------------------------------------
QString MainWindow::sortHoundCalls(QString t, int isort, int max_dB)
{
/* Called from "houndCallers()" to sort the list of calling stations by
 * specified criteria.
 *
 * QString "t" contains a list of Hound callers read from file "houndcallers.txt".
 *    isort=0: random    (shuffled order)
 *          1: Call
 *          2: Grid
 *          3: SNR       (reverse order)
 *          4: Distance  (reverse order)
*/

  QMap<QString,QString> map;
  QStringList lines,lines2;
  QString msg,houndCall,t1;
  QString ABC{"ABCDEFGHIJKLMNOPQRSTUVWXYZ _"};
  QList<int> list;
  int i,j,k,m,n,nlines;
  bool bReverse=(isort >= 3);

  isort=qAbs(isort);
// Save only the most recent transmission from each caller.
  lines = t.split("\n");
  nlines=lines.length()-1;
  for(i=0; i<nlines; i++) {
    msg=lines.at(i);                        //key = callsign
    if(msg.mid(13,1)==" ") msg=msg.mid(0,13) + "...." + msg.mid(17);
    houndCall=msg.split(" ").at(0);         //value = "call grid snr freq dist age"
    map[houndCall]=msg;
  }

  j=0;
  t="";
  for(auto a: map.keys()) {
    t1=map[a].split(" ",QString::SkipEmptyParts).at(2);
    int nsnr=t1.toInt();                         // get snr
    if(nsnr <= max_dB) {                         // keep only if snr in specified range
      if(isort==1) t += map[a] + "\n";
      if(isort==3 or isort==4) {
        i=2;                                           // sort Hound calls by snr
        if(isort==4) i=4;                              // sort Hound calls by distance
        t1=map[a].split(" ",QString::SkipEmptyParts).at(i);
        n=1000*(t1.toInt()+100) + j;                   // pack (snr or dist) and index j into n
      }

      if(isort==2) {                                   // sort Hound calls by grid
        t1=map[a].split(" ",QString::SkipEmptyParts).at(1);
        if(t1=="....") t1="ZZ99";
        int i1=ABC.indexOf(t1.mid(0,1));
        int i2=ABC.indexOf(t1.mid(1,1));
        n=100*(26*i1+i2)+t1.mid(2,2).toInt();
        n=1000*n + j;                                 // pack ngrid and index j into n
      }

      list.insert(j,n);                               // add n to list at [j]
      lines2.insert(j,map[a]);                        // add map[a] to lines2 at [j]
      j++;
    }
  }

  if(isort>1) {
    if(bReverse) {
      qSort(list.begin(),list.end(),qGreater<int>());
    } else {
      qSort(list.begin(),list.end());
    }
  }

  if(isort>1) {
    for(i=0; i<j; i++) {
      k=list[i]%1000;
      n=list[i]/1000 - 100;
      t += lines2.at(k) + "\n";
    }
  }

  int nn=lines2.length();
  if(isort==0) {                                      // shuffle Hound calls to random order
    int a[nn];
    for(i=0; i<nn; i++) {
      a[i]=i;
    }
    for(i=nn-1; i>-1; i--) {
      j=(i+1)*double(qrand())/RAND_MAX;
      m=a[j];
      a[j]=a[i];
      a[i]=m;
      t += lines2.at(m) + "\n";
    }
  }

  int i0=t.indexOf("\n") + 1;
  m_nSortedHounds=0;
  if(i0 > 0) {
    m_nSortedHounds=qMin(t.length(),m_Nlist*i0)/i0; // Number of sorted & displayed Hounds
  }
  m_houndCallers=t.mid(0,m_Nlist*i0);

  return m_houndCallers;
}

//------------------------------------------------------------------------------
void MainWindow::selectHound(QString line)
{
/* Called from doubleClickOnCall() in DXpedition Fox mode.
 * QString "line" is a user-selected line from left text window.
 * The line may be selected by double-clicking; alternatively, hitting
 * <Enter> is equivalent to double-clicking on the top-most line.
*/

  if(line.length()==0) return;
  QString houndCall=line.split(" ",QString::SkipEmptyParts).at(0);

// Don't add a call already enqueued or in QSO
  if(ui->textBrowser4->toPlainText().indexOf(houndCall) >= 0) return;

  QString houndGrid=line.split(" ",QString::SkipEmptyParts).at(1);  // Hound caller's grid
  QString rpt=line.split(" ",QString::SkipEmptyParts).at(2);        // Hound SNR

  m_houndCallers=m_houndCallers.remove(line+"\n");      // Remove t from sorted Hound list
  m_nSortedHounds--;
  ui->decodedTextBrowser->setText(m_houndCallers);   // Populate left window with Hound callers
  QString t1=houndCall + "          ";
  QString t2=rpt;
  if(rpt.mid(0,1) != "-" and rpt.mid(0,1) != "+") t2="+" + rpt;
  if(t2.length()==2) t2=t2.mid(0,1) + "0" + t2.mid(1,1);
  t1=t1.mid(0,12) + t2;
  ui->textBrowser4->displayFoxToBeCalled(t1,"#ffffff");  // Add hound call and rpt to tb4
  t1=t1 + " " + houndGrid;                               // Append the grid
  m_houndQueue.enqueue(t1);                              // Put this hound into the queue
  writeFoxQSO(" Sel:  " + t1);
  QTextCursor cursor = ui->textBrowser4->textCursor();
  cursor.setPosition(0);                                 // Scroll to top of list
  ui->textBrowser4->setTextCursor(cursor);
}

//------------------------------------------------------------------------------
void MainWindow::houndCallers()
{
/* Called from decodeDone(), in DXpedition Fox mode.  Reads decodes from file
 * "houndcallers.txt", ignoring any that are not addressed to MyCall, are already
 * in the stack, or with whom a QSO has been started.  Others are considered to
 * be Hounds eager for a QSO.  We add caller information (Call, Grid, SNR, Freq,
 * Distance, Age, and Continent) to a list, sort the list by specified criteria,
 * and display the top N_Hounds entries in the left text window.
*/
  QFile f(m_config.temp_dir().absoluteFilePath("houndcallers.txt"));
  if(f.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QTextStream s(&f);
    QString t="";
    QString line,houndCall,paddedHoundCall;
    m_nHoundsCalling=0;
    int nTotal=0;  //Total number of decoded Hounds calling Fox in 4 most recent Rx sequences

// Read and process the file of Hound callers.
    while(!s.atEnd()) {
      line=s.readLine();
      nTotal++;
      int i0=line.indexOf(" ");
      houndCall=line.mid(0,i0);
      paddedHoundCall=houndCall + " ";
      //Don't list a hound already in the queue
      if(!ui->textBrowser4->toPlainText().contains(paddedHoundCall)) {
        if(m_loggedByFox[houndCall].contains(m_lastBand))   continue;   //already logged on this band
        if(m_foxQSO.contains(houndCall)) continue;   //still in the QSO map
        QString countryName,continent;
        bool callWorkedBefore,countryWorkedBefore;
        m_logBook.match(/*in*/houndCall,/*out*/countryName,callWorkedBefore,countryWorkedBefore);
        int i1=countryName.lastIndexOf(";");
        continent=countryName.mid(i1+2,-1);

//If we are using a directed CQ, ignore Hound calls that do not comply.
        QString CQtext=ui->comboBoxCQ->currentText();
        if(CQtext.length()==5 and (continent!=CQtext.mid(3,2))) continue;
        int nCallArea=-1;
        if(CQtext.length()==4) {
          for(int i=houndCall.length()-1; i>0; i--) {
            if(houndCall.mid(i,1).toInt() > 0) nCallArea=houndCall.mid(i,1).toInt();
            if(houndCall.mid(i,1)=="0") nCallArea=0;
            if(nCallArea>=0) break;
          }
          if(nCallArea!=CQtext.mid(3,1).toInt()) continue;
        }
//This houndCall passes all tests, add it to the list.
        t = t + line + "  " + continent + "\n";
        m_nHoundsCalling++;                // Number of accepted Hounds to be sorted
      }
    }
    if(m_msgAvgWidget != NULL and m_msgAvgWidget->isVisible()) {
      m_msgAvgWidget->foxLabCallers(nTotal);
    }

// Sort and display accumulated list of Hound callers
    if(t.length()>30) {
      m_isort=ui->comboBoxHoundSort->currentIndex();
      QString t1=sortHoundCalls(t,m_isort,m_max_dB);
      ui->decodedTextBrowser->setText(t1);
    }
    f.close();
  }
}

void MainWindow::foxRxSequencer(QString msg, QString houndCall, QString rptRcvd)
{
/* Called from "readFromStdOut()" to process decoded messages of the form
 * "myCall houndCall R+rpt".
 *
 * If houndCall matches a callsign in one of our active QSO slots, we
 * prepare to send "houndCall RR73" to that caller.
*/
  if(m_foxQSO.contains(houndCall)) {
    m_foxQSO[houndCall].rcvd=rptRcvd.mid(1);  //Save report Rcvd, for the log
    m_foxQSO[houndCall].tFoxRrpt=m_tFoxTx;    //Save time R+rpt was received
    writeFoxQSO(" Rx:   " + msg.trimmed());
  } else {
    for(QString hc: m_foxQSO.keys()) {        //Check for a matching compound call
      if(hc.contains("/"+houndCall) or hc.contains(houndCall+"/")) {
        m_foxQSO[hc].rcvd=rptRcvd.mid(1);  //Save report Rcvd, for the log
        m_foxQSO[hc].tFoxRrpt=m_tFoxTx;    //Save time R+rpt was received
        writeFoxQSO(" Rx:   " + msg.trimmed());
      }
    }
  }
}

void MainWindow::foxTxSequencer()
{
/* Called from guiUpdate at the point where an FT8 Fox-mode transmission
 * is to be started.
 *
 * Determine what the Tx message(s) will be for each active slot, call
 * foxgen() to generate and accumulate the corresponding waveform.
*/

  qint64 now=QDateTime::currentMSecsSinceEpoch()/1000;
  QStringList list1;                        //Up to NSlots Hound calls to be sent RR73
  QStringList list2;                        //Up to NSlots Hound calls to be sent a report
  QString fm;                               //Fox message to be transmitted
  QString hc,hc1,hc2;                       //Hound calls
  QString t,rpt;
  qint32  islot=0;
  qint32  n1,n2,n3;

  m_tFoxTx++;                               //Increment Fox Tx cycle counter

  //Is it time for a stand-alone CQ?
  if(m_tFoxTxSinceCQ >= m_foxCQtime and ui->cbMoreCQs->isChecked()) {
    fm=ui->comboBoxCQ->currentText() + " " + m_config.my_callsign();
    if(!fm.contains("/")) {
      //If Fox is not a compound callsign, add grid to the CQ message.
      fm += " " + m_config.my_grid().mid(0,4);
      m_fullFoxCallTime=now;
    }
    m_tFoxTx0=m_tFoxTx;                     //Remember when we sent a CQ
    islot++;
    foxGenWaveform(islot-1,fm);
    goto Transmit;
  }
//Compile list1: up to NSLots Hound calls to be sent RR73
  for(QString hc: m_foxQSO.keys()) {           //Check all Hound calls: First priority
    if(m_foxQSO[hc].tFoxRrpt<0) continue;
    if(m_foxQSO[hc].tFoxRrpt - m_foxQSO[hc].tFoxTxRR73 > 3) {
      //Has been a long time since we sent RR73
      list1 << hc;                          //Add to list1
      m_foxQSO[hc].tFoxTxRR73 = m_tFoxTx;   //Time RR73 is sent
      m_foxQSO[hc].nRR73++;                 //Increment RR73 counter
      if(list1.size()==m_Nslots) goto list1Done;
    }
  }

  for(QString hc: m_foxQSO.keys()) {           //Check all Hound calls: Second priority
    if(m_foxQSO[hc].tFoxRrpt<0) continue;
    if(m_foxQSO[hc].tFoxTxRR73 < 0) {
      //Have not yet sent RR73
      list1 << hc;                          //Add to list1
      m_foxQSO[hc].tFoxTxRR73 = m_tFoxTx;   //Time RR73 is sent
      m_foxQSO[hc].nRR73++;                 //Increment RR73 counter
      if(list1.size()==m_Nslots) goto list1Done;
    }
  }

  for(QString hc: m_foxQSO.keys()) {           //Check all Hound calls: Third priority
    if(m_foxQSO[hc].tFoxRrpt<0) continue;
    if(m_foxQSO[hc].tFoxTxRR73 <= m_foxQSO[hc].tFoxRrpt) {
      //We received R+rpt more recently than we sent RR73
      list1 << hc;                          //Add to list1
      m_foxQSO[hc].tFoxTxRR73 = m_tFoxTx;   //Time RR73 is sent
      m_foxQSO[hc].nRR73++;                 //Increment RR73 counter
      if(list1.size()==m_Nslots) goto list1Done;
    }
  }

list1Done:
//Compile list2: Up to Nslots Hound calls to be sent a report.
  for(int i=0; i<m_foxQSOinProgress.count(); i++) {
    //First do those for QSOs in progress
    hc=m_foxQSOinProgress.at(i);
    if((m_foxQSO[hc].tFoxRrpt < 0) and (m_foxQSO[hc].ncall < m_maxStrikes)) {
      //Sent him a report and have not received R+rpt: call him again
      list2 << hc;                          //Add to list2
      if(list2.size()==m_Nslots) goto list2Done;
    }
  }

  while(!m_houndQueue.isEmpty()) {
    //Start QSO with a new Hound
    t=m_houndQueue.dequeue();             //Fetch new hound from queue
    int i0=t.indexOf(" ");
    hc=t.mid(0,i0);                       //hound call
    list2 << hc;                          //Add new Hound to list2
    m_foxQSOinProgress.enqueue(hc);       //Put him in the QSO queue
    m_foxQSO[hc].grid=t.mid(16,4);        //Hound grid
    rpt=t.mid(12,3);                      //report to send Hound
    m_foxQSO[hc].sent=rpt;                //Report to send him
    m_foxQSO[hc].ncall=0;                 //Start a new Hound
    m_foxQSO[hc].nRR73 = 0;               //Have not sent RR73
    m_foxQSO[hc].rcvd = -99;              //Have not received R+rpt
    m_foxQSO[hc].tFoxRrpt = -1;           //Have not received R+rpt
    m_foxQSO[hc].tFoxTxRR73 = -1;         //Have not sent RR73
    rm_tb4(hc);                           //Remove this Hound from tb4
    if(list2.size()==m_Nslots) goto list2Done;
    if(m_foxQSO.count()>=2*m_Nslots) goto list2Done;
  }

list2Done:
  n1=list1.size();
  n2=list2.size();
  n3=qMax(n1,n2);
  if(n3>m_Nslots) n3=m_Nslots;
  for(int i=0; i<n3; i++) {
    hc1="";
    fm="";
    if(i<n1 and i<n2) {
      hc1=list1.at(i);
      hc2=list2.at(i);
      m_foxQSO[hc2].ncall++;
      fm = Radio::base_callsign(hc1) + " RR73; " + Radio::base_callsign(hc2) +
          " <" + m_config.my_callsign() + "> " + m_foxQSO[hc2].sent;
    }
    if(i<n1 and i>=n2) {
      hc1=list1.at(i);
      fm = Radio::base_callsign(hc1) + " " + m_baseCall + " RR73";                 //Standard FT8 message
    }

    if(hc1!="") {
      // Log this QSO!
      m_hisCall=hc1;
      m_hisGrid=m_foxQSO[hc1].grid;
      m_rptSent=m_foxQSO[hc1].sent;
      m_rptRcvd=m_foxQSO[hc1].rcvd;
      QDateTime logTime {QDateTime::currentDateTimeUtc ()};
      QString thc1=(m_hisCall + "   ").mid(0,6);
      if(m_hisCall.contains("/")) thc1=m_hisCall;
      QString logLine=logTime.toString("yyyy-MM-dd hh:mm") + " " + thc1 + "  " +
          m_hisGrid + "  " + m_rptSent + "  " + m_rptRcvd + " " + m_lastBand;
      if(m_msgAvgWidget != NULL and m_msgAvgWidget->isVisible()) {
        m_msgAvgWidget->foxAddLog(logLine);
      }
      on_logQSOButton_clicked();
      writeFoxQSO(" Log:  " + logLine.mid(17));
      m_foxRateQueue.enqueue(now);             //Add present time in seconds to Rate queue.
      m_loggedByFox[hc1] += (m_lastBand + " ");
    }

    if(i<n2 and fm=="") {
      hc2=list2.at(i);
      m_foxQSO[hc2].ncall++;
      fm = Radio::base_callsign(hc2) + " " + m_baseCall + " " + m_foxQSO[hc2].sent; //Standard FT8 message
    }
    islot++;
    foxGenWaveform(islot-1,fm);                             //Generate tx waveform
  }

  if(islot < m_Nslots) {
    //At least one slot is still open
    if(islot==0 or ((m_tFoxTx-m_tFoxTx0>=4) and ui->cbMoreCQs->isChecked())) {
      //Roughly every 4th Tx sequence, put a CQ message in an otherwise empty slot
      fm=ui->comboBoxCQ->currentText() + " " + m_config.my_callsign();
      if(!fm.contains("/")) {
        fm += " " + m_config.my_grid().mid(0,4);
        m_tFoxTx0=m_tFoxTx;                                //Remember when we send a CQ
        m_fullFoxCallTime=now;
      }
      islot++;
      foxGenWaveform(islot-1,fm);
    }
  }

Transmit:
  foxcom_.nslots=islot;
  foxcom_.nfreq=ui->TxFreqSpinBox->value();
  if(m_config.split_mode()) foxcom_.nfreq = foxcom_.nfreq - m_XIT;  //Fox Tx freq
  QString foxCall=m_config.my_callsign() + "         ";
  strncpy(&foxcom_.mycall[0], foxCall.toLatin1(),12);   //Copy Fox callsign into foxcom_
  foxgen_();
  m_tFoxTxSinceCQ++;

  for(QString hc: m_foxQSO.keys()) {               //Check for strikeout or timeout
    if(m_foxQSO[hc].ncall>=m_maxStrikes) m_foxQSO[hc].ncall++;
    bool b1=((m_tFoxTx - m_foxQSO[hc].tFoxRrpt) > 2*m_maxFoxWait) and
        (m_foxQSO[hc].tFoxRrpt > 0);
    bool b2=((m_tFoxTx - m_foxQSO[hc].tFoxTxRR73) > m_maxFoxWait) and
        (m_foxQSO[hc].tFoxTxRR73>0);
    bool b3=(m_foxQSO[hc].ncall >= m_maxStrikes+m_maxFoxWait);
    bool b4=(m_foxQSO[hc].nRR73 >= m_maxStrikes);
    if(b1 or b2 or b3 or b4) {
      m_foxQSO.remove(hc);
      m_foxQSOinProgress.removeOne(hc);
    }
  }

  while(!m_foxRateQueue.isEmpty()) {
    qint64 age = now - m_foxRateQueue.head();
    if(age < 3600) break;
    m_foxRateQueue.dequeue();
  }
  if(m_msgAvgWidget != NULL and m_msgAvgWidget->isVisible()) {
    m_msgAvgWidget->foxLabRate(m_foxRateQueue.size());
    m_msgAvgWidget->foxLabQueued(m_foxQSOinProgress.count());
  }
}

void MainWindow::rm_tb4(QString houndCall)
{
  if(houndCall=="") return;
  QString t="";
  QString tb4=ui->textBrowser4->toPlainText();
  QStringList list=tb4.split("\n");
  int n=list.size();
  int j=0;
  for (int i=0; i<n; i++) {
    if(j>0) t += "\n";
    QString line=list.at(i);
    if(!line.contains(houndCall + " ")) {
      j++;
      t += line;
    }
  }
  t.replace("\n\n","\n");
  ui->textBrowser4->setText(t);
}

void MainWindow::doubleClickOnFoxQueue(Qt::KeyboardModifiers modifiers)
{
  if(modifiers==9999) return;                               //Silence compiler warning
  QTextCursor cursor=ui->textBrowser4->textCursor();
  cursor.setPosition(cursor.selectionStart());
  QString houndCall=cursor.block().text().mid(0,12).trimmed();
  rm_tb4(houndCall);
  writeFoxQSO(" Del:  " + houndCall);
  QQueue<QString> tmpQueue;
  while(!m_houndQueue.isEmpty()) {
    QString t=m_houndQueue.dequeue();
    QString hc=t.mid(0,12).trimmed();
    if(hc != houndCall) tmpQueue.enqueue(t);
  }
  m_houndQueue.swap(tmpQueue);
}

void MainWindow::foxGenWaveform(int i,QString fm)
{
//Generate and accumulate the Tx waveform
  fm += "                                        ";
  fm=fm.mid(0,40);
  if(fm.mid(0,3)=="CQ ") m_tFoxTxSinceCQ=-1;

  QString txModeArg;
  txModeArg.sprintf("FT8fox %d",i+1);
  ui->decodedTextBrowser2->displayTransmittedText(fm.trimmed(), txModeArg,
        ui->TxFreqSpinBox->value()+60*i,m_config.color_ReceivedMsg(),m_bFastMode);
  foxcom_.i3bit[i]=0;
  if(fm.indexOf("<")>0) foxcom_.i3bit[i]=1;
  strncpy(&foxcom_.cmsg[i][0],fm.toLatin1(),40);   //Copy this message into cmsg[i]
  if(i==0) m_fm1=fm;
  QString t;
  t.sprintf(" Tx%d:  ",i+1);
  writeFoxQSO(t + fm.trimmed());
}

void MainWindow::writeFoxQSO(QString msg)
{
  QString t;
  t.sprintf("%3d%3d%3d",m_houndQueue.count(),m_foxQSOinProgress.count(),m_foxQSO.count());
  QFile f {m_config.writeable_data_dir ().absoluteFilePath ("FoxQSO.txt")};
  if (f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
    QTextStream out(&f);
    out << QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd hh:mm:ss")
        << "  " << fixed << qSetRealNumberPrecision (3) << (m_freqNominal/1.e6)
        << t << msg << endl;
    f.close();
  } else {
    MessageBox::warning_message (this, tr("File Open Error"),
      tr("Cannot open \"%1\" for append: %2").arg(f.fileName()).arg(f.errorString()));
  }
}

/*################################################################################### */
void MainWindow::foxTest()
{
  QFile f("steps.txt");
  if(!f.open(QIODevice::ReadOnly | QIODevice::Text)) return;

  QFile fdiag("diag.txt");
  if(!fdiag.open(QIODevice::WriteOnly | QIODevice::Text)) return;

  QTextStream s(&f);
  QTextStream sdiag(&fdiag);
  QString line;
  QString t;
  QString msg;
  QString hc1;
  QString rptRcvd;
  qint32 n=0;

  while(!s.atEnd()) {
    line=s.readLine();
    if(line.length()==0) continue;
    if(line.mid(0,4).toInt()==0) line="                                     " + line;
    if(line.contains("NSlots")) {
      n=line.mid(44,1).toInt();
      ui->sbNslots->setValue(n);
    }
    if(line.contains("Sel:")) {
      t=line.mid(43,6) + "       " + line.mid(54,4) + "   " + line.mid(50,3);
      selectHound(t);
    }

    if(line.contains("Del:")) {
      int i0=line.indexOf("Del:");
      hc1=line.mid(i0+6);
      int i1=hc1.indexOf(" ");
      hc1=hc1.mid(0,i1);
      rm_tb4(hc1);
      writeFoxQSO(" Del:  " + hc1);
      QQueue<QString> tmpQueue;
      while(!m_houndQueue.isEmpty()) {
        t=m_houndQueue.dequeue();
        QString hc=t.mid(0,6).trimmed();
        if(hc != hc1) tmpQueue.enqueue(t);
      }
      m_houndQueue.swap(tmpQueue);
    }
    if(line.contains("Rx:"))  {
      msg=line.mid(43);
      t=msg.mid(24);
      int i0=t.indexOf(" ");
      hc1=t.mid(i0+1);
      int i1=hc1.indexOf(" ");
      hc1=hc1.mid(0,i1);
      int i2=qMax(msg.indexOf("R+"),msg.indexOf("R-"));
      if(i2>20) {
        rptRcvd=msg.mid(i2,4);
        foxRxSequencer(msg,hc1,rptRcvd);
      }
    }
    if(line.contains("Tx1:")) {
      foxTxSequencer();
    } else {
      t.sprintf("%3d %3d %3d %3d %5d   ",m_houndQueue.count(),
                m_foxQSOinProgress.count(),m_foxQSO.count(),
                m_loggedByFox.count(),m_tFoxTx);
      sdiag << t << line.mid(37).trimmed() << "\n";
    }
  }
}
