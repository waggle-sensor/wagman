namespace Logger
{
    void begin(const char *name);
    void end();
    void log(const char *str);
};

extern bool logging;

