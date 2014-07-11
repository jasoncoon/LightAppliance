// Pointer to a function taking no parameters and returning void
typedef void (*ptr2Function)();

// Pointer to a function taking two rgb24 parameters and returning void
typedef void (*ptr2SetFunction)(rgb24, rgb24);

// Mode definition structure
typedef struct {
    char *name;
    ptr2Function function;
} 
NAMED_FUNCTION;

enum USER_INTERACTION_CODE {
    UICODE_HOME, UICODE_SELECT, UICODE_LEFT, UICODE_RIGHT
};




