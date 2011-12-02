#ifndef USLEEP_H
#define USLEEP_H

#include <qthread.h>

class I : public QThread
{
public:
	static void sleep(unsigned long secs) {
		QThread::sleep(secs);
	}
	static void msleep(unsigned long msecs) {
		QThread::msleep(msecs);
	}
	static void usleep(unsigned long usecs) {
		QThread::usleep(usecs);
	}
};

#endif // USLEEP_H
