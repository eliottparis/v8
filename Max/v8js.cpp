/*
// Copyright (c) 2015 Eliott Paris, CICM, Universite Paris 8.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "MaxV8.h"

void ext_main(void *r)
{
    using namespace cicm;
    
    t_class *c = class_new("v8js",
                           (method)MaxV8::NewInstance,
                           (method)MaxV8::FreeInstance,
                           (long)sizeof(MaxV8), 0L, A_GIMME, 0);
    
    class_addmethod(c, (method)MaxV8::Assist,           "assist",   	A_CANT,     0);
    class_addmethod(c, (method)MaxV8::Loadbang,         "loadbang",     A_CANT,     0);
    class_addmethod(c, (method)MaxV8::Read,             "compile",      A_DEFSYM,   0);
    class_addmethod(c, (method)MaxV8::Bang,             "bang",         0,          0);
    class_addmethod(c, (method)MaxV8::Int,              "int",          A_LONG,     0);
    class_addmethod(c, (method)MaxV8::Float,            "float",        A_FLOAT,    0);
    class_addmethod(c, (method)MaxV8::Anything,         "anything",     A_GIMME,    0);
    class_addmethod(c, (method)MaxV8::OpenEditor,       "dblclick",     A_CANT,     0);
    class_addmethod(c, (method)MaxV8::OpenEditor,       "open",         0,          0);
    class_addmethod(c, (method)MaxV8::EditorClosed,     "edclose",      A_CANT,     0);
    class_addmethod(c, (method)MaxV8::EditorSaved,      "edsave",       A_CANT,     0);
    
    // global v8 init
    MaxV8::Init();
    
    // register a method on max quit to shut v8 down
    quittask_install((method)MaxV8::Release, NULL);
    
    class_register(CLASS_BOX, c);
    MaxV8::obj_class = c;
}
