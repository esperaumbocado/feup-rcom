#include "link_layer.h"

void logStateTransition(int oldState, int newState)
{
    if (oldState != newState)
    {
        printf(
            "====================\n"
            "STATE TRANSITION\n"
            "%d -> %d\n" 
            "====================\n", 
            oldState, newState
        );
    }
}