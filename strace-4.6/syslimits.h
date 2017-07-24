/*******************************************************************************
module:   syslimits
author:   digimokan
date:     21 JUL 2017 (created)
purpose:  obtain various limits of the current operating system
ref:      Advanced Programming In The UNIX Environment, 3rd Ed, Section 2.6
*******************************************************************************/

#ifndef SYSLIMITS_H
#define SYSLIMITS_H

// return the max num of open files - at any given time - on this system
long max_open_files (void);

#endif // SYSLIMITS_H

