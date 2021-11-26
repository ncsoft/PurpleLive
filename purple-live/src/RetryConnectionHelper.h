#pragma once

#include <QTimer>
#include <functional>
#include <vector>

enum eRetryConnectionState {
	eRCS_None,
	eRCS_Retrying,
	eRCS_Successed,
	eRCS_Failed,
};

class RetryConnectionHelper
{
public:
	explicit RetryConnectionHelper();
	virtual ~RetryConnectionHelper();

	void SetRetryTimes(std::vector<int> time_list);
	void Init();
	void Stop();
	bool IsRetrying();
	void Retry(std::function<void(bool timeout)> callback);

private:
	bool retrying = false;
	int times_index = 0;
	int current_waiting_time = 0;
	QTimer* timer = nullptr;
	std::vector<int> times;
};

