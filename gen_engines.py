#!/usr/bin/python

import os
import glob

engines = glob.glob("engines/*")



for e in engines:
    
    e = e.replace("engines/","")
    
    print( "Version : " + e )
    
    if not os.path.exists( "copilot-" + e + ".zip" ):
        os.system("cd engines ; zip -r -9  ../copilot-" + e + ".zip " + e + " ")
    
        os.system("xxd -i copilot-" + e + ".zip copilot-" + e + ".h")


header=""
for e in engines:
    e = e.replace("engines/","")
    
    header += "#include \"copilot-" + e + ".h\"\n"
    


header += "static engine_info engines[] = {\n"

for e in engines:
    e = e.replace("engines/","")
    
    e_file = e.replace(".","_")
    
    header += "   { .version = \"" + e + "\" , .bin = copilot_" + e_file + "_zip, .size = &copilot_" + e_file + "_zip_len },\n"

header +="   { .version = 0 }\n"
header +="};\n"

cur_header = ""

if os.path.exists("engine_list.h"):
    with open( "engine_list.h", "r" ) as f:
        cur_header = f.read()

if not cur_header == header:
    with open( "engine_list.h", "w" ) as f:
        f.write( header )
