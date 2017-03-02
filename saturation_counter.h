enum TwoBitState {STRONGLY_NOT_TAKEN, WEAKLY_NOT_TAKEN, WEAKLY_TAKEN,
    STRONGLY_TAKEN};
class TwoBitSaturationCounter{
public:
	TwoBitSaturationCounter();
	TwoBitState CounterState();
	void Update(bool branch_taken);
private:
	TwoBitState counter;
};
TwoBitSaturationCounter::TwoBitSaturationCounter(void){
	counter = STRONGLY_NOT_TAKEN;
}

TwoBitState TwoBitSaturationCounter::CounterState(){
	return counter;
}

void TwoBitSaturationCounter::Update(bool branch_taken){
	TwoBitState temp = STRONGLY_NOT_TAKEN;
	if(counter == STRONGLY_NOT_TAKEN){
		if (branch_taken) temp = WEAKLY_NOT_TAKEN;
		else temp = STRONGLY_NOT_TAKEN;
	}
	else if(counter == WEAKLY_NOT_TAKEN){
		if (branch_taken) temp = WEAKLY_TAKEN;
		else temp = STRONGLY_NOT_TAKEN;
	}
	else if(counter == WEAKLY_TAKEN){
		if (branch_taken) temp = STRONGLY_TAKEN;
		else temp = WEAKLY_NOT_TAKEN;
	}
	else if(counter == STRONGLY_TAKEN){
		if (!branch_taken) temp = WEAKLY_TAKEN;
		else temp = STRONGLY_TAKEN;
	}
	counter = temp;
}