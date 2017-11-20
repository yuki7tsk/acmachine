/*
* dawg.cpp
*/

#include "dawg.h"

#include <deque>

#include <algorithm>

#include <cctype>


DAWG::DAWG(void) {
	setupInitialState();
	resetState();
}

void DAWG::setupInitialState(void) {
	transitions.clear();
	transitions.push_back(std::map<alphabet, state_edgelabel>());
	spointer.clear();
	spointer.push_back(initialState());
	torb.clear();
	torb.push_back(true);
	datoac.clear();
	datoac.push_back(initialState());
}


int DAWG::transfer(const state current, const alphabet & c, const int pors, const bool ignore_case) {
  std::map<alphabet, state_edgelabel>::iterator itr;
  state newstate;
  
  if (ignore_case) {
    itr = transitions[current].find(toupper(c));
    if (itr == transitions[current].end())
      itr = transitions[current].find(tolower(c));
  }
  else {
    itr = transitions[current].find(c);
  }
  
  if (itr == transitions[current].end()) {
    return -1;
  }

  if (pors == -1) 
	  newstate = itr->second.first;
  else {
	  if (pors != itr->second.second) {
		  return -1;
	  }
	  newstate = itr->second.first;
  }

  return newstate;
}


DAWG::state DAWG::update(state sink, char c) {
    state currentsink = sink;
    state newsink;
    state_edgelabel totra;

    if(transfer(currentsink, c) > 0){
      //std::cout << "not make primary" << std::endl;
      if(transfer(currentsink, c, true) > 0){
		newsink = transfer(currentsink, c, true);
		torb[newsink] = true;
		if(datoac[newsink] == -1)
		  dawgtoacm(true, newsink);
		return newsink;
	  }
      else{
		newsink = transfer(currentsink, c, false);
		torb.push_back(true);
		dawgtoacm(true);
		return split(currentsink, newsink, c);
	  }
    }
    else{
      //std::cout << "make primary" << std::endl;
      newsink = transitions.size();
      totra.first = newsink; totra.second = true;
      transitions.push_back(std::map<alphabet, state_edgelabel>());
      transitions[currentsink].insert(std::make_pair(c, totra));
      torb.push_back(true);
      dawgtoacm(true);

      spointer.push_back(initialState());

      state currentstate = currentsink;
      int sfstate = -1;

      while((currentstate != initialState()) && (sfstate == -1)){
		currentstate = spointer[currentstate];
		//std::cout << "current state: " << currentstate << std::endl;
		if((transfer(currentstate, c, true) == -1) && (transfer(currentstate, c, false) == -1)){
			//std::cout << "while: not edge" << std::endl;
			totra.first = newsink; totra.second = false;
			transitions[currentstate].insert(std::make_pair(c, totra));
			//std::cout << "check" << std::endl;
		}
		else if(transfer(currentstate, c, true) > 0){
		  //std::cout << "while: primary" << std::endl;
		  sfstate = transfer(currentstate, c, true);
		}
		else if(transfer(currentstate, c, false) > 0){
		  //std::cout << "while: secondary" << std::endl;
		  state child = transfer(currentstate, c, false);
		  torb.push_back(false);
		  dawgtoacm(false);
		  sfstate = split(currentstate, child, c);
		  //std::cout << "while: secondary sfstate: " << sfstate << std::endl;
		}
      }

      if(sfstate == -1)
		sfstate = initialState();

      spointer[newsink] = sfstate;
      return newsink;
    }
}

DAWG::state DAWG::split(state parentstate, state childstate, char c) {
  
  state newchildstate;
  std::map<alphabet, state_edgelabel>::iterator itr;
  newchildstate = transitions.size();
  transitions.push_back(std::map<alphabet, state_edgelabel>());
  state_edgelabel  totra;
  totra.first = newchildstate; totra.second = true;
  
  itr = transitions[parentstate].find(c);
  transitions[parentstate].erase(itr);
  transitions[parentstate].insert(std::make_pair(c, totra));
  transitions[newchildstate].insert(transitions[childstate].begin(), transitions[childstate].end());
  for(auto & assoc:transitions[newchildstate]) 
    assoc.second.second = false;

  spointer.push_back(spointer[childstate]);
  spointer[childstate] = newchildstate;

  state currentstate;
  currentstate = parentstate;
  totra.first = newchildstate; totra.second = false;

  while(currentstate != initialState()){
    currentstate = spointer[currentstate];
    itr = transitions[currentstate].find(c);
    if(itr->second.second == false){
      transitions[currentstate].erase(itr);
      transitions[currentstate].insert(std::make_pair(c, totra));
      
    }
    else{
      break;
    }
    
  }

  return newchildstate;
  
}



std::vector<DAWG::fstates> DAWG::getFailStates(int start, std::string patt) {
  std::vector<fstates> failstates;
  state activestate = initialState();
  state cstate, currentstate;
  state tonum;
  state stnum;
  fstates tmp;
  int pattsize = patt.length();
  std::deque<state> st;
  std::deque<int> st_num;

  for (int i = 0; i <= pattsize; i++) {
    if (i > start) {
      st.push_front(activestate);
      st_num.push_front(i);
    }


    if ((transfer(activestate, patt[i]) == -1))
      break;

    stnum = transfer(activestate, patt[i]);
    
    activestate = stnum;
  }

  std::deque<state> queue;
  std::deque<int> queue_num;
  std::vector<state> mark;


  while (!st.empty()) {
    cstate = st.front();
    tonum = st_num.front();
    st.pop_front();
	st_num.pop_front();
    queue.push_back(cstate);

    while (!queue.empty()) {
      currentstate = queue.front();
      queue.pop_front();

	  auto itr = std::find(mark.begin(), mark.end(), currentstate);
	  if (itr == mark.end()) {
		mark.push_back(currentstate);
		if ((torb[currentstate] == true)) {
			tmp.first = datoac[currentstate];
			tmp.second = tonum;
			failstates.push_back(tmp);
	    }
	    else {
			for (state s = 0; s < spointer.size(); s++) {
				if(spointer[s] == currentstate)
					queue.push_back(s);
			}
		}
	  }

	}

  }
	  return failstates;
}


std::vector<DAWG::state> DAWG::getOutStates(std::string patt) {
  std::vector<state> outstates;
  state activestate = initialState();
  state cstate, currentstate;
  state tonum;
  int stnum;
  int pattsize = patt.length();
  std::deque<state> st;

  for (int i = 0; i < pattsize; i++) {
    
    stnum = transfer(activestate, patt[i]);

    if (stnum == -1)
      break;
    
    activestate = stnum;
  }


  if (stnum == -1)
	  return outstates;
  
  std::deque<state> queue;
  queue.push_back(activestate);

  while (!queue.empty()) {

    cstate = queue.front();
    queue.pop_front();
    if(torb[cstate] == true)
      outstates.push_back(datoac[cstate]);
    
    for (state s = 0; s < spointer.size(); s++) {
      if(spointer[s] == cstate)
		queue.push_back(s);
    }

  }
  return outstates;
}


