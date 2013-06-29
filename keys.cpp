#include "keys.h"

#include <boost/algorithm/string.hpp>
#include <fstream>

#include "PtOpenGL.h"
#include "IPManager.h"
#include "Audio.h"

namespace {
	int glfwStrToKey(string str) {
		int key;
		if(str == "GLFW_KEY_SPACE") key = GLFW_KEY_SPACE;
		else if(str == "GLFW_KEY_ESC") key = GLFW_KEY_ESC;
		else if(str == "GLFW_KEY_F1") key = GLFW_KEY_F1;
		else if(str == "GLFW_KEY_F2") key = GLFW_KEY_F2;
		else if(str == "GLFW_KEY_F3") key = GLFW_KEY_F3;
		else if(str == "GLFW_KEY_F4") key = GLFW_KEY_F4;
		else if(str == "GLFW_KEY_F5") key = GLFW_KEY_F5;
		else if(str == "GLFW_KEY_F6") key = GLFW_KEY_F6;
		else if(str == "GLFW_KEY_F7") key = GLFW_KEY_F7;
		else if(str == "GLFW_KEY_F8") key = GLFW_KEY_F8;
		else if(str == "GLFW_KEY_F9") key = GLFW_KEY_F9;
		else if(str == "GLFW_KEY_F10") key = GLFW_KEY_F10;
		else if(str == "GLFW_KEY_F11") key = GLFW_KEY_F11;
		else if(str == "GLFW_KEY_F12") key = GLFW_KEY_F12;
		else if(str == "GLFW_KEY_F13") key = GLFW_KEY_F13;
		else if(str == "GLFW_KEY_F15") key = GLFW_KEY_F15;
		else if(str == "GLFW_KEY_UP") key = GLFW_KEY_UP;
		else if(str == "GLFW_KEY_DOWN") key = GLFW_KEY_DOWN;
		else if(str == "GLFW_KEY_LEFT") key = GLFW_KEY_LEFT;
		else if(str == "GLFW_KEY_RIGHT") key = GLFW_KEY_RIGHT;
		else if(str == "GLFW_KEY_LSHIFT") key = GLFW_KEY_LSHIFT;
		else if(str == "GLFW_KEY_RSHIFT") key = GLFW_KEY_RSHIFT;
		else if(str == "GLFW_KEY_LCTRL") key = GLFW_KEY_LCTRL;
		else if(str == "GLFW_KEY_RCTRL") key = GLFW_KEY_RCTRL;
		else if(str == "GLFW_KEY_LALT") key = GLFW_KEY_LALT;
		else if(str == "GLFW_KEY_RALT") key = GLFW_KEY_RALT;
		else if(str == "GLFW_KEY_LSUPER") key = GLFW_KEY_LSUPER;
		else if(str == "GLFW_KEY_RSUPER") key = GLFW_KEY_RSUPER;
		else if(str == "GLFW_KEY_TAB") key = GLFW_KEY_TAB;
		else if(str == "GLFW_KEY_ENTER") key = GLFW_KEY_ENTER;
		else if(str == "GLFW_KEY_BACKSPACE") key = GLFW_KEY_BACKSPACE;
		else if(str == "GLFW_KEY_INSERT") key = GLFW_KEY_INSERT;
		else if(str == "GLFW_KEY_DEL") key = GLFW_KEY_DEL;
		else if(str == "GLFW_KEY_PAGEUP") key = GLFW_KEY_PAGEUP;
		else if(str == "GLFW_KEY_PAGEDOWN") key = GLFW_KEY_PAGEDOWN;
		else if(str == "GLFW_KEY_HOME") key = GLFW_KEY_HOME;
		else if(str == "GLFW_KEY_END") key = GLFW_KEY_END;
		else if(str == "GLFW_KEY_KP_0") key = GLFW_KEY_KP_0;
		else if(str == "GLFW_KEY_KP_1") key = GLFW_KEY_KP_1;
		else if(str == "GLFW_KEY_KP_2") key = GLFW_KEY_KP_2;
		else if(str == "GLFW_KEY_KP_3") key = GLFW_KEY_KP_3;
		else if(str == "GLFW_KEY_KP_4") key = GLFW_KEY_KP_4;
		else if(str == "GLFW_KEY_KP_5") key = GLFW_KEY_KP_5;
		else if(str == "GLFW_KEY_KP_6") key = GLFW_KEY_KP_6;
		else if(str == "GLFW_KEY_KP_7") key = GLFW_KEY_KP_7;
		else if(str == "GLFW_KEY_KP_8") key = GLFW_KEY_KP_8;
		else if(str == "GLFW_KEY_KP_9") key = GLFW_KEY_KP_9;
		else if(str == "GLFW_KEY_KP_DIVIDE") key = GLFW_KEY_KP_DIVIDE;
		else if(str == "GLFW_KEY_KP_MULTIPLY") key = GLFW_KEY_KP_MULTIPLY;
		else if(str == "GLFW_KEY_KP_SUBTRACT") key = GLFW_KEY_KP_SUBTRACT;
		else if(str == "GLFW_KEY_KP_ADD") key = GLFW_KEY_KP_ADD;
		else if(str == "GLFW_KEY_KP_DECIMAL") key = GLFW_KEY_KP_DECIMAL;
		else if(str == "GLFW_KEY_KP_EQUAL") key = GLFW_KEY_KP_EQUAL;
		else if(str == "GLFW_KEY_KP_ENTER") key = GLFW_KEY_KP_ENTER;
		else if(str == "GLFW_KEY_KP_NUM_LOCK") key = GLFW_KEY_KP_NUM_LOCK;
		else if(str == "GLFW_KEY_CAPS_LOCK") key = GLFW_KEY_CAPS_LOCK;
		else if(str == "GLFW_KEY_SCROLL_LOCK") key = GLFW_KEY_SCROLL_LOCK;
		else if(str == "GLFW_KEY_PAUSE") key = GLFW_KEY_PAUSE;
		else if(str == "GLFW_KEY_MENU") key = GLFW_KEY_MENU;
		else {
			key = str[0];
		}
		return key;
	}
}


KeyBinding::KeyBinding(GLPresenter* pres) : pres(pres) {
	ifstream kfs("keys.ini");
	string temp;
	while(getline(kfs, temp)) {
		handleBinding(temp);
	}
	ifstream sfs("startup.ini");
	while(getline(sfs, temp)) {
		handleStartup(temp);
	}
}

void KeyBinding::handleBinding(string bind) {
	if(bind.find('#') != bind.npos) bind.erase(bind.find('#'));
	vector<string> splitVec;
	boost::split(splitVec, bind, boost::is_any_of(" =\t\n"), boost::token_compress_on);
	if(bind.empty()) return;
	if(splitVec.size()!=2) {
		WARN("Malformed keybinding: " << bind);
		return;
	}

	int key = glfwStrToKey(splitVec[1]);
	binding[key] = splitVec[0];
}

void KeyBinding::handleStartup(string start) {
	if(start.find('#') != start.npos) start.erase(start.find('#'));
	vector<string> splitVec;
	boost::split(splitVec, start, boost::is_any_of(" =\t\n"), boost::token_compress_on);
	if(start.empty()) return;
	if(splitVec.size()!=1) {
		WARN("Malformed startup command: " << start);
		return;
	}
	performAction(splitVec[0]);
}

void KeyBinding::handleEvent(int key, int action) {
	if(action == GLFW_RELEASE) {
		if(binding.find(key) != binding.end()) {
			performAction(binding[key]);
		} else {
			DEBUG("Unbound key: " << key);
		}
	}
}

bool KeyBinding::isFunCall(const string& fun, string action) {
	size_t pos = action.find_first_of('(');
	action = action.substr(0, pos);
	return action == fun;
}

string KeyBinding::decodeParam(string action, unsigned num) {
	int lpos = action.npos;
	for(unsigned i=0; i<=num; ++i) {
		int lpos = action.find_first_of("(,");
		RT_ASSERT(lpos != action.npos, "Invalid keybinding/autostart function call: " << action);
		action = action.substr(lpos+1);
	}
	int rpos = action.find_first_of("),");
	RT_ASSERT(rpos != action.npos && rpos-lpos-1 > 0, "Invalid keybinding/autostart function call: " << action);
	return action.substr(0, rpos);
}

float KeyBinding::decodeFloatParam(const string& fun, unsigned num) {
	string numStr = decodeParam(fun, num);
	return (float)atof(numStr.c_str());
}

int KeyBinding::decodeIntParam(const string& fun, unsigned num) {
	string numStr = decodeParam(fun, num);
	return atoi(numStr.c_str());
}

void KeyBinding::performAction(string action) {
	if(action == "exit") {
		pres->closeEvent();
	} else if(action == "toggleFullscreen") {
		pres->switchFullscreen();
	} else if(action == "toggleVSync") {
		pres->switchVSync();
	} else if(action == "toggleRGBRange") {
		pres->toggleRGBRange();
	} else if(action == "captureScreenshot") {
		pres->saveScreenshot();
	} else if(action == "setDisplayHz") {
		pres->setDisplayHz();
	} else if(isFunCall("resizeTo", action)) {
		pres->resizeTo(decodeIntParam(action));
	} else if(action == "switchAspect") {
		pres->switchAspect();
	} 
	// image processing
	else if(action == "toggleLocalContrast") {
		pres->getIPManager()->toggleFilter(IPManager::LOCAL_CONTRAST);
	} else if(isFunCall("adjustLocalContrast", action)) {
		pres->getIPManager()->adjustContrastIncrease(decodeFloatParam(action));
	} else if(action == "toggleGammaCorrection") {
		pres->getIPManager()->toggleFilter(IPManager::GAMMA_CORRECTION);
	} else if(isFunCall("adjustGamma", action)) {
		pres->getIPManager()->adjustGamma(decodeFloatParam(action));
	}
	// scaling & AA
	else if(action == "changeScaling") {
		pres->changeScaling();
	} if(isFunCall("setScaling", action)) {
		pres->setScaling(decodeIntParam(action));
	} else if(action == "changeAA") {
		pres->changeAA();
	} else if(isFunCall("setAA", action)) {
		pres->setAA(decodeIntParam(action));
	} else if(isFunCall("adjustPredicate", action)) {
		pres->adjustPredicate(decodeFloatParam(action, 0), decodeFloatParam(action, 1));
	} else if(isFunCall("toggleOneToNScaling", action)) {
		pres->toggleOneToNScaling(decodeFloatParam(action));
	} 
	// audio
	else if(action == "toggleSound") {
		AudioRenderer::get().toggleMuted();
	} else if(action == "toggleExpandStereo") {
		AudioRenderer::get().toggleExpandStereo();
	} else if(action == "toggleDTSDecoding") {
		AudioRenderer::get().toggleDTS();
	}  else if(action == "toggleDDDecoding") {
		AudioRenderer::get().toggleDD();
	} else if(isFunCall("adjustVolume", action)) {
		AudioRenderer::get().adjustVolume(decodeFloatParam(action));
	} else if(isFunCall("boostAudioLevels", action)) {
		AudioRenderer::get().adjustBoost(decodeIntParam(action));
	}
	// testing
	else if(action == "testAA") {
		pres->testAA();
	}
	// unrecognized command
	else {
		WARN("Unknown action: "<< action);
	}
}
