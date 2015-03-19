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

#include <default_gui_model.h>
#include <gen_sine.h>
//#include </home/user/Projects/rtxi/plugins/data_recorder/data_recorder.h>
//#include "~/RTXI/rtxi/plugins/data_recorder/data_recorder.h"
#include <math.h>
#include <string>

class HFAC : public DefaultGUIModel {
	Q_OBJECT
	
	public:
		HFAC(void);
		virtual ~HFAC(void);
	
		void execute(void);
		void customizeGUI(void);
	
		enum HFACmode_t {
			HFACON, HFACOFF,
		};
	
		enum APmode_t {
			POSNEG, NEGPOS,
		};
	
	public slots:
	
	signals: // custom signals
	
	protected:
		virtual void update(DefaultGUIModel::update_flags_t);
	
	private:
		void initParameters();
		void initStimulus(); // creates AP stim and HFAC stimuli
	
		double APamp;
		double APwidth;
		double APdelay;
		std::vector<double> APStimwave;
		int APStimidx;
		int nAPStimsamples;
		APmode_t APmode;
	
		double HFACfreq;
		double HFACamp;
		GeneratorSine HFACwave;
		HFACmode_t HFACmode;
	
		bool APstimGO;
		bool hfacison;
		bool protocolGO;
		QString dFile;
//		bool recordon;

		double duration;
		long long count;
		double systime;
		double dt;
	
		// QT components
		QPushButton *APBttn;
		QPushButton *hfacBttn;
		QPushButton *protocolBttn;
	
	private slots:
		void startHFAC(bool);
		void sendAPStim();
		void runProtocol();
		void updateAPStimMode(int);
//		void toggleRecord(bool);
};
