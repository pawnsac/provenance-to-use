def python_process_detector(line, L1):
    """
    identifies all python processes listed in the log file
    and appends their process_id number (string) to a global list
    of all such processes, called L1

    input: line - line of text from the log file; L1 - global list
    output: none. 
    """

    if " EXECVE " in line and "/python" in line:
       line  = line.replace("\n", "")
       a = "["
       b = "]"
       sq_bracket_cont_str = line.split(a,1)[-1].split(b)[0]
       t = eval(sq_bracket_cont_str) # tuple
       if "python" in t:
          L = line.split(" ")
          process_id = L[3]
          if process_id not in L1:
             L1.append(process_id)

def pickling_detector(line, L2):
    """
    identifies all python processes that have initiated a pickle process.
    all such python process are recorded in the "L2"
    using their process id.

    input: line - line of text from the log file; L2 - global list
    output: none.     
    """
    if " READ " in line and "/usr/lib/python" in line:
       line = line.replace("\n", "")
       if line.endswith("pickle.py") or line.endswith("pickle.pyc"):
          L = line.split(" ")
          python_pickle_process_id = L[1]
          if python_pickle_process_id not in L2:
             L2.append(python_pickle_process_id)

def names_and_process_id_of_READ_pickle_files_dumped_to_disk_detector(line, D1):
    """
    identifies all .p (pickled) files and their process id that were read in.
    input: line - line of text from the log file; D1 - global dictionary
    output: none
    """
    if " READ " in line:
       line = line.replace("\n","")
       if line.endswith(".p"):
          L = line.split(" ")
          process_id = L[1]
          index = line.rfind("/") #index of last occurrence of "/"
          file_name = line[index+1:]
          if file_name not in D1:
             D1.update({process_id:file_name})

def names_and_process_id_of_WRITE_pickle_files_dumped_to_disk_detector(line, D2):
    """
    identifies all .p (pickled) files and their process id that were written to.
    input: line - line of text from the log file; D2 - global dictionary
    output: none
    """
    if " WRITE " in line:
       line = line.replace("\n","")
       if line.endswith(".p"):
          L = line.split(" ")
          process_id = L[1]
          index = line.rfind("/") #index of last occurrence of "/"
          file_name = line[index+1:]
          if file_name not in D2:
             D2.update({process_id:file_name})

def warning_messages_generator(L1, L2, D1, D2):
    for i in range(len(L1)): #iterating over all python process ids
        pid = L1[i]
        if pid in L2: #if pid (a python process) initiated the pickle library
           if pid not in D1: #if pid is not a key in a dictionary D1
              if pid not in D2: #if pid is not a key in dictionary D2
                 print ""
                 print "Warning: pickling was initiated but not used!"
                 print "No pickled files (read from or written to) have been included with this package!"       
                 print ""
              else:
                 print ""
                 print "Message: this package includes only pickled (.p) file(s) that were written to disk"
                 print "by the python process. No reading of pickled files has occurred."
                 print ""
           else:
              if pid not in D2:
                 print ""
                 print "Message: this package includes only pickled (.p) file(s) that were read in"
                 print "by the python process. No writing of pickled files to disk has occurred." 
                 print ""
 
def deliver_messages(logfile):
    L1 = [] #python_process_list
    L2 = [] #python_process_ids_of_pickelings_list
    D1 = {} #names_and_process_id_of_all_READ_pickle_files_in_log_file_dict
    D2 = {} #names_and_process_id_of_all_WRITE_pickle_files_in_log_file_dict
    with open(logfile) as f:
         for line in f:
             python_process_detector(line, L1)
             pickling_detector(line, L2)
             names_and_process_id_of_READ_pickle_files_dumped_to_disk_detector(line, D1)
             names_and_process_id_of_WRITE_pickle_files_dumped_to_disk_detector(line, D2)
    warning_messages_generator(L1, L2, D1, D2)
