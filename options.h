#ifndef _OPTIONS_H_
#define _OPTIONS_H_

bool parse_options(int & argc, char * argv[]);

bool enabled(const char * option);
bool disabled(const char * option);
void enable(const char * option);
void disable(const char * option);

bool visible(const char * c);

#endif
