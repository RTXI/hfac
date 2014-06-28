/*
 Copyright (C) 2011 Georgia Institute of Technology
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 */

#include <HFAC.h>
#include <math.h>
#include <default_gui_model.h>
#include <main_window.h>
#include <QtGui>

extern "C" Plugin::Object *
createRTXIPlugin(void)
{
  return new HFAC();
}

#define TWOPI 6.28318531

static HFAC::variable_t vars[] =
  {
    { "AP Stim", "Current for AP stimulus", DefaultGUIModel::OUTPUT, },
    { "HFAC Signal", "Current for HFAC conduction block",
        DefaultGUIModel::OUTPUT, },
    { "AP Stim Amplitude", "AP Stim Amplitude", DefaultGUIModel::PARAMETER
        | DefaultGUIModel::DOUBLE, },
    { "AP Stim Delay (ms)", "AP Stim Delay (ms)", DefaultGUIModel::PARAMETER
        | DefaultGUIModel::DOUBLE, },
    { "AP Stim Width (ms)", "AP Stim Width", DefaultGUIModel::PARAMETER
        | DefaultGUIModel::DOUBLE, },
    { "HFAC Freq", "HFAC Freq (kHz)", DefaultGUIModel::PARAMETER
        | DefaultGUIModel::DOUBLE, },
    { "HFAC Amplitude", "HFAC Amplitude", DefaultGUIModel::PARAMETER
        | DefaultGUIModel::DOUBLE, },
    { "Trial Duration (s)", "Trial Duration (s)", DefaultGUIModel::PARAMETER
        | DefaultGUIModel::DOUBLE, },
    { "Data File Name", "Name of HDF File", DefaultGUIModel::COMMENT },
    { "Time (s)", "Time (s)", DefaultGUIModel::STATE, }, };

static size_t num_vars = sizeof(vars) / sizeof(DefaultGUIModel::variable_t);

HFAC::HFAC(void) :
  DefaultGUIModel("HFAC", ::vars, ::num_vars)
{
  setWhatsThis(
      "<p><b>HFAC Conduction Block:</b></p><p>Implements a protocol for conduction block experiments in nerve"
      " fibers using high frequency AC current waveforms. Action potentials in the nerve are evoked by a "
      "biphasic square pulse and the HFAC signal is a sinusoidal waveform of a specific frequency. You may "
      "select whether the positive or negative component of the AP stimulus pulse occurs first. You "
      "can send a single stimulation pulse and toggle the HFAC signal on/off. You can also run a timed "
      "protocol in which the HFAC signal is enabled and the AP stimulation pulse is automatically triggered "
      "after a specified delay.  Synchronizing this module with the Data Recorder will create separate trials "
      "in the HDF5 file corresponding to the protocols initiated with the \"Run\" button.</p>");
  initParameters();
  initStimulus();
  createGUI(vars, num_vars); // this is required to create the GUI
  update( INIT);
  refresh();
  printf("Starting HFAC Module:\n");
}

HFAC::~HFAC(void)
{
}

void
HFAC::execute(void)
{
  systime = count * dt; // time in seconds

  if (protocolGO) // running timed protocol
    {
      output(1) = HFACwave.get();
      if (systime > APdelay && systime <= APdelay + 2 * APwidth)
        {
          output(0) = APStimwave[APStimidx++];
        }
      else
        {
          output(0) = 0;
        }
      if (systime >= duration)
        {
          pause(true);
          update( PAUSE);
          if (recordon)
            DataRecorder::stopRecording();
          protocolGO = false;
          APBttn->setEnabled(true);
          APstimGO = false;
          hfacBttn->setDown(false);
          HFACmode = HFACOFF;
        }
      count++; // increment time

    }
  else // manual mode
    {
      if (HFACmode == HFACON)
        {
          output(1) = HFACwave.get();
        }
      else if (HFACmode == HFACOFF)
        {
          output(1) = 0;
        }

      if (APstimGO)
        {
          output(0) = APStimwave[APStimidx++];
          if (APStimidx >= nAPStimsamples)
            {
              APStimidx = 0;
              APstimGO = false;
              APBttn->setEnabled(true);
            }
        }
      else
        {
          output(0) = 0;
        }
      count++; // increment time

    }
}

void
HFAC::update(DefaultGUIModel::update_flags_t flag)
{
  switch (flag)
    {
  case INIT:
    setParameter("AP Stim Amplitude", QString::number(APamp));
    setParameter("AP Stim Width (ms)", QString::number(APwidth * 1e3)); // show in ms, use in s
    setParameter("AP Stim Delay (ms)", QString::number(APdelay * 1e3)); // show in ms, use in s
    setParameter("HFAC Freq", QString::number(HFACfreq));
    setParameter("HFAC Amplitude", QString::number(HFACamp));
    setParameter("Trial Duration (s)", QString::number(duration));
    setState("Time (s)", systime);
    setComment("Data File Name", dFile);
    DataRecorder::openFile(dFile);
    break;
  case MODIFY:
    APamp = getParameter("AP Stim Amplitude").toDouble();
    APwidth = getParameter("AP Stim Width (ms)").toDouble() / 1e3; // get in ms, convert to s
    APdelay = getParameter("AP Stim Delay (ms)").toDouble() / 1e3; // get in ms, convert to s
    HFACfreq = getParameter("HFAC Freq").toDouble();
    HFACamp = getParameter("HFAC Amplitude").toDouble();
    duration = getParameter("Trial Duration (s)").toDouble();
    if (duration <= APdelay)
      {
        QMessageBox::critical(
            this,
            "HFAC",
            tr(
                "You may want to lengthen the duration of your trial given your AP stimulus delay!\n"));
      }
    initStimulus();
    if (recordon)
      {
        QString newdFile = getComment("Data File Name");
        int c = QString::compare(newdFile, dFile);
        if (c != 0)
          {
            printf("Saving to new file: %s\n", dFile.latin1());
            dFile = newdFile;
            DataRecorder::openFile(dFile);
          }
      }
    break;
  case PERIOD:
    dt = RT::System::getInstance()->getPeriod() * 1e-9; // time in seconds
    initStimulus();
  case PAUSE:
    output(0) = 0.0;
    output(1) = 0.0;
    break;
  case UNPAUSE:
    systime = 0;
    count = 0;
    if (recordon)
      DataRecorder::startRecording();
    break;
  default:
    break;
    }
}

void
HFAC::initParameters()
{
  APamp = 1;
  APwidth = .4e-3; // s
  APdelay = .150e-3; // s
  APStimidx = 0;
  APmode = POSNEG;

  HFACfreq = 10; // Hz
  HFACamp = 3;

  HFACmode = HFACOFF;
  APstimGO = false;

  recordon = true;
  dFile = "default.h5";
  duration = 2; // s
  dt = RT::System::getInstance()->getPeriod() * 1e-9; // s
  systime = 0;
  count = 0;
  output(0) = 0;
  output(1) = 0;
}

void
HFAC::initStimulus()
{
  // AP stim, biphasic square wave
  APStimwave.clear();
  nAPStimsamples = floor(APdelay / dt) + 2 * floor(APwidth / dt) + 1;
  for (int i = 0; i < floor(APdelay / dt); i++)
    {
      APStimwave.push_back(0); // initial delay
    }
  if (APmode == POSNEG)
    {
      for (int i = 0; i < floor(APwidth / dt); i++)
        {
          APStimwave.push_back(APamp); // positive part
        }
      for (int i = 0; i < floor(APwidth / dt); i++)
        {
          APStimwave.push_back(-APamp); // negative part
        }
    }
  else if (APmode == NEGPOS)
    {
      for (int i = 0; i < floor(APwidth / dt); i++)
        {
          APStimwave.push_back(-APamp); // positive part
        }
      for (int i = 0; i < floor(APwidth / dt); i++)
        {
          APStimwave.push_back(APamp); // negative part
        }
    }
  APStimwave.push_back(0);

  /*
   printf("AP Stim:\n");
   for (int i = 0; i < nAPStimsamples; i++)
   {
   printf("%f \n", APStimwave[i]);
   }
   */

  // HFAC stim, sine wave
  HFACwave.init(HFACfreq * 1000, HFACamp, dt);
  /*
   printf("HFAC Wave:\n");
   for (int i = 0; i < HFACwave2.numSamples(); i++)
   {
   printf("%f \n", HFACwave2.get());
   }
   */
}

void
HFAC::startHFAC(bool hfacison)
{
  if (hfacison)
    {
      HFACmode = HFACON;
      printf("HFAC on\n");
    }
  else
    {
      HFACmode = HFACOFF;
      printf("HFAC off\n");
    }
  output(0) = 0;
  output(1) = 0;

}

void
HFAC::sendAPStim()
{
  // send AP stim
  APstimGO = true;
  printf("Sent AP Stim\n");
  APBttn->setEnabled(false);
}

void
HFAC::runProtocol()
{

  APstimGO = false;
  protocolGO = true;
  printf("Ran protocol\n");
  APBttn->setEnabled(false);
  pause(false);
  update( UNPAUSE);
}

void
HFAC::updateAPStimMode(int index)
{
  if (index == 0)
    {
      APmode = POSNEG;
      printf("AP Stim mode is now pos to neg\n");
      update( MODIFY);
    }
  else if (index == 1)
    {
      APmode = NEGPOS;
      printf("AP Stim mode is now neg to pos\n");
      update( MODIFY);
    }
}

void
HFAC::toggleRecord(bool on)
{
  recordon = on;
  if (on)
    printf("Data recording is ON\n");
  else
    printf("Data recording is OFF\n");
}

void
HFAC::createGUI(DefaultGUIModel::variable_t *var, int size)
{

  QBoxLayout *layout = new QVBoxLayout(this);

  QHButtonGroup *bttnBox = new QHButtonGroup("HFAC Functions", this);
  APBttn = new QPushButton("Single AP Stim", bttnBox);
  APBttn->setToggleButton(false);
  QToolTip::add(APBttn, "Send a single AP stimulus");
  QObject::connect(APBttn, SIGNAL(clicked()), this, SLOT(sendAPStim()));
  hfacBttn = new QPushButton("Enable HFAC", bttnBox);
  hfacBttn->setToggleButton(true);
  QToolTip::add(hfacBttn, "Turn HFAC on/off");
  QObject::connect(hfacBttn, SIGNAL(toggled(bool)), this, SLOT(startHFAC(bool)));
  protocolBttn = new QPushButton("Run", bttnBox);
  protocolBttn->setToggleButton(false);
  QToolTip::add(protocolBttn, "Run Protocol");
  QObject::connect(protocolBttn, SIGNAL(clicked()), this, SLOT(runProtocol()));

  QHButtonGroup *modeBox = new QHButtonGroup("AP Stim Polarity", this);
  modeBox->setRadioButtonExclusive(true);
  QRadioButton *posButton = new QRadioButton("+/-", modeBox);
  posButton->setChecked(true);
  QRadioButton *negButton = new QRadioButton("-/+", modeBox);
  QObject::connect(modeBox,SIGNAL(clicked(int)),this,SLOT(updateAPStimMode(int)));
  QToolTip::add(posButton, "Set biphasic pulse to pos then neg");
  QToolTip::add(negButton, "Set biphaisc pulse to neg then pos");

  QHButtonGroup *optionRow = new QHButtonGroup("Data Recorder", this);
  QToolTip::add(optionRow, "Select whether to sync with the data recorder");
  QCheckBox *recordCheckBox = new QCheckBox("Sync Data", optionRow);
  recordCheckBox->setChecked(true); // set some defaults
  recordCheckBox->setEnabled(true);
  QObject::connect(recordCheckBox, SIGNAL(toggled(bool)), this, SLOT(toggleRecord(bool)));

  // add custom GUI components to layout above default_gui_model components
  layout->addWidget(bttnBox);
  layout->addWidget(modeBox);

  QScrollView *sv = new QScrollView(this);
  sv->setResizePolicy(QScrollView::AutoOneFit);
  layout->addWidget(sv);

  QWidget *viewport = new QWidget(sv->viewport());
  sv->addChild(viewport);
  QGridLayout *scrollLayout = new QGridLayout(viewport, 1, 2);

  size_t nstate = 0, nparam = 0, nevent = 0, ncomment = 0;
  for (size_t i = 0; i < num_vars; i++)
    {
      if (vars[i].flags & (PARAMETER | STATE | EVENT | COMMENT))
        {
          param_t param;

          param.label = new QLabel(vars[i].name, viewport);
          scrollLayout->addWidget(param.label, parameter.size(), 0);
          param.edit = new DefaultGUILineEdit(viewport);
          scrollLayout->addWidget(param.edit, parameter.size(), 1);

          QToolTip::add(param.label, vars[i].description);
          QToolTip::add(param.edit, vars[i].description);

          if (vars[i].flags & PARAMETER)
            {
              if (vars[i].flags & DOUBLE)
                {
                  param.edit->setValidator(new QDoubleValidator(param.edit));
                  param.type = PARAMETER | DOUBLE;
                }
              else if (vars[i].flags & UINTEGER)
                {
                  QIntValidator *validator = new QIntValidator(param.edit);
                  param.edit->setValidator(validator);
                  validator->setBottom(0);
                  param.type = PARAMETER | UINTEGER;
                }
              else if (vars[i].flags & INTEGER)
                {
                  param.edit->setValidator(new QIntValidator(param.edit));
                  param.type = PARAMETER | INTEGER;
                }
              else
                param.type = PARAMETER;
              param.index = nparam++;
              param.str_value = new QString;
            }
          else if (vars[i].flags & STATE)
            {
              param.edit->setReadOnly(true);
              param.edit->setPaletteForegroundColor(Qt::darkGray);
              param.type = STATE;
              param.index = nstate++;
            }
          else if (vars[i].flags & EVENT)
            {
              param.edit->setReadOnly(true);
              param.type = EVENT;
              param.index = nevent++;
            }
          else if (vars[i].flags & COMMENT)
            {
              param.type = COMMENT;
              param.index = ncomment++;
            }

          parameter[vars[i].name] = param;
        }
    }

  layout->addWidget(optionRow);

  QHBox *hbox1 = new QHBox(this);
  pauseButton = new QPushButton("Pause", hbox1);
  pauseButton->setToggleButton(true);
  QObject::connect(pauseButton,SIGNAL(toggled(bool)),this,SLOT(pause(bool)));
  QPushButton *modifyButton = new QPushButton("Modify", hbox1);
  QObject::connect(modifyButton,SIGNAL(clicked(void)),this,SLOT(modify(void)));
  QPushButton *unloadButton = new QPushButton("Unload", hbox1);
  QObject::connect(unloadButton,SIGNAL(clicked(void)),this,SLOT(exit(void)));
  layout->addWidget(hbox1);

  show();

}
