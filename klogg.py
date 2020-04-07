import sys
import re
import subprocess
import os
import logging
with open('test.txt', 'w') as f:
    f.write('dan')
    
LOGGER = logging.getLogger()
LOGGER.setLevel(logging.DEBUG)
handler = logging.StreamHandler(open('klogg.py.log', 'a'))
handler.setLevel(logging.DEBUG)
LOGGER.addHandler(handler)


SOURCE_INSIGHT_COMMAND = r'"C:\Program Files (x86)\Source Insight 4.0\sourceinsight4.exe" +{line} "{source}"'
#NOTEPAD_P_P_COMMAND = r'"C:\Program Files (x86)\Notepad++\notepad++.exe" "{source}" -n{line}'
COMMAND = SOURCE_INSIGHT_COMMAND
WORKSPACE = r'C:\Users\dkinsbur\Perforce\dkinsbur-MOBL1_OTM'
BASE_FOLDER = os.path.join(WORKSPACE, 'fw', 'bin','dummy')
        
def double_click_line(line):
    LOGGER.info("[DBL_CLICK] line:'{}'".format(line))
    #m = re.search(",\"[^:\"]+:\\d+\"", line)
    match = re.search(r'","(?P<source>\.\./\.\./[\w\./]+):(?P<line>\d+)', line)
    if match:
        src = match.groupdict()['source']
        line = match.groupdict()['line']
        source = os.path.abspath(os.path.join(BASE_FOLDER, src))
        
        cmd = COMMAND.format(source=source, line=line)
        LOGGER.info("[Exec] '{}'".format(cmd))
        subprocess.Popen(cmd)
    
if __name__ == '__main__':
    try:
        assert len(sys.argv) == 2
        double_click_line(sys.argv[2])
    except:
        import traceback
        LOGGER.error(traceback.format_exc())
    