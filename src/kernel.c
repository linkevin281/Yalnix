



void KernelStart(char* cmd_args[]) {
    /**
     * 1. Initialize all queues()
     * 2. Initialize all trap vectors()
     * 
    */
}






























































int allocateFrame(){
    /**
     * If there are frames in empty_frames:
     *      pop the first frame off of empty_frames
     *      Return the number of this frame
     * Else:
     *      Return -1 (this could potentially be modified by evicting an existing frame - something to pursue once base functionality is working)
    */
}

int deallocateFrame(int frame_index){
    /**
     * Add a node with frame_index to empty_frames
     * NOTE: user is responsible for flushing TLB after calling!
    */
}

int runNewProcess(){
    /**
     * Grab the first pcb off of the ready queue. This is the pcb to_switch
     * If current process has more instructions to run AND is in "running" state (indicating it has been running with no issue)
     *      add it to the ready queue
     * Else if current process in zombie state (indicating it is dead):
     *      Move it into its parent's queue of zombies
     * Call KernelContextSwitch(KCSwitch) with to_switch, to switch this process in
     * Change current_process to the pcb of to_switch
    */
}