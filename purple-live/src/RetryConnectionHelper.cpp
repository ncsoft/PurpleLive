#include "RetryConnectionHelper.h"
#include "easylogging++.h"

RetryConnectionHelper::RetryConnectionHelper()
{

}

RetryConnectionHelper::~RetryConnectionHelper()
{
	Init();
}

void RetryConnectionHelper::SetRetryTimes(std::vector<int> time_list)
{
	times.clear();
	times.assign(time_list.begin(), time_list.end());
}

void RetryConnectionHelper::Init()
{
	retrying = false;
	times_index = 0;
	if (timer)
	{
		timer->deleteLater();
		timer = nullptr;
	}
}

void RetryConnectionHelper::Stop()
{
	Init();
}

bool RetryConnectionHelper::IsRetrying()
{
	return retrying;
}

void RetryConnectionHelper::Retry(std::function<void(bool timeout)> callback)
{
	if (times_index >= times.size())
	{
		elogd("[RetryConnectionHelper::Retry] timeout");
		Stop();
		callback(true);
		return;
	}

	retrying = true;
	current_waiting_time = times[times_index];
	elogd("[RetryConnectionHelper::Retry] times_index(%d) current_waiting_time(%d) waiting", times_index, current_waiting_time);

	if (timer)
		timer->deleteLater();
	timer = new QTimer();
	timer->setSingleShot(true);
	timer->start(current_waiting_time * 1000);
	QObject::connect(timer, &QTimer::timeout, [this, callback]() {
		elogd("[RetryConnectionHelper::Retry] times_index(%d) current_waiting_time(%d) callback", times_index, current_waiting_time);
		times_index++;
		timer->deleteLater();
		timer = nullptr;
		callback(false);
	});
}