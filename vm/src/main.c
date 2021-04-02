#include "frame.h"
int main(){
	frame *frame = parse_frame("D 3");

	frame_dump(frame);
}
