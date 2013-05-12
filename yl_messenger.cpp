#include "yl_messenger.h"
#include "yl_sensor.h"

//#define YL_SERIAL_DEBUG

namespace yeelink
{
#define YEELINK_VERSION ("v1.0")

	yl_messenger::yl_messenger()
		: version_(YEELINK_VERSION)
	{}

	yl_messenger::yl_messenger(uint8_t sock)
		: EthernetClient(sock)
		, version_(YEELINK_VERSION)
	{}

	yl_messenger::yl_messenger(const String &api_key, const String &host)
		: EthernetClient()
		, api_key_(api_key)
		, host_(host)
		, version_(YEELINK_VERSION)
	{}

	yl_messenger::yl_messenger(uint8_t sock, const String &api_key, const String &host)
		: EthernetClient(sock)
		, api_key_(api_key)
		, host_(host)
		, version_(YEELINK_VERSION)
	{}

	void yl_messenger::set_api_key(const String &api_key)
	{
		api_key_ = api_key;
	}

	const String& yl_messenger::get_api_key() const
	{
		return api_key_;
	}

	String& yl_messenger::get_api_key()
	{
		return get_api_key();
	}

	void yl_messenger::set_host(const String &host)
	{
		host_ = host;
	}

	const String& yl_messenger::get_host() const
	{
		return host_;
	}

	String& yl_messenger::get_host()
	{
		return get_host();
	}

	void yl_messenger::set_version(const String &version)
	{
		version_ = version;
	}

	const String& yl_messenger::get_version() const
	{
		return version_;
	}

	String& yl_messenger::get_version()
	{
		return get_version();
	}

	bool yl_messenger::connect_yl()
	{
		return connect(&host_[0], 80);
	}

	bool yl_messenger::request_post(const yl_sensor &sensor, const yl_data_point &dp)
	{
		String data = dp.to_string();
		if (data.length() == 0)
		{
			return false;
		}
		String temp("POST /");
		temp += version_;
		temp += "/device/";
		if (!send(temp)
			|| !print(sensor.get_device()->get_id())
			|| !send("/sensor/") 
			|| !print(sensor.get_id())
			|| !send("/datapoints HTTP/1.1\r\nHost:")
			|| !send(host_)
			|| !send("\r\nAccept:*/*\r\nU-ApiKey:")
			|| !send(api_key_)
			|| !send("\r\nContent-Length:")
			|| !println(data.length())
			|| !send("Content-Type:application/x-www-form-urlencoded\r\nConnection:close\r\n\r\n")
			|| !send(data))
		{
#ifdef YL_SERIAL_DEBUG
			Serial.println("send error : 108");
#endif
			return false;
		}
		return true;
	}

	bool yl_messenger::get_request_result()
	{
		String temp;
		if (!recv_ln(temp))
		{
#ifdef YL_SERIAL_DEBUG
			Serial.println("recv error : 122");
#endif
			return false;
		}
		int start_index = temp.indexOf(' ');
		temp = temp.substring(start_index + 1);
		temp.trim();
		int end_index = temp.indexOf(' ');
		temp = temp.substring(0, end_index);
		temp.trim();
		return temp[0] == '2';
	}

	bool yl_messenger::request_get(const yl_sensor &sensor, const String &key)
	{
		String temp("GET /");
		temp += version_;
		temp += "device/";
		if (!send(temp)
			|| !print(sensor.get_device()->get_id())
			|| !send("/sensor/")
			|| !print(sensor.get_id())
			|| !send("/datapoints/"))
		{
#ifdef YL_SERIAL_DEBUG
			Serial.println("send error : 144");
#endif
			return false;
		}
		if ((key.length() && !send(key))
			|| !send(" HTTP/1.1\r\nHost:")
			|| !send(host_)
			|| !send("\r\nU-ApiKey:")
			|| !send(api_key_)
			|| !send("\r\nConnection:close\r\n\r\n"))
		{
#ifdef YL_SERIAL_DEBUG
			Serial.println("send error : 156");
#endif
			return false;
		}
		return true;
	}

	bool yl_messenger::recv_get_data(String &data)
	{
		if (!get_request_result())
		{
#ifdef YL_SERIAL_DEBUG
			Serial.println("send error : 168");
#endif
			return false;
		}
		String temp;
		String content_length("content-length:");
		if (!recv_ln_when(content_length, temp))
		{
			return false;
		}
		int length = 0;
		int last_index = temp.indexOf("\r\n");
		temp = temp.substring(0, last_index);
		temp.trim();
		length = temp.toInt();
		if (length <= 0)
		{
			return false;
		}
		if (!recv_ln_when("\r\n", data))
		{
#ifdef YL_SERIAL_DEBUG
			Serial.println("recv error : 193");
#endif
			return false;
		}
		return false;
	}

	bool yl_messenger::recv_ln(String &data)
	{
		int result = 0;
		for (uint8_t step=0;step < 2;)
		{
			if ((result = read()) <= 0) 
			{
#ifdef YL_SERIAL_DEBUG
				Serial.println("read error : 190");
#endif
				return false;
			}
			if (step == 0 && result == '\r')
			{
				step = 1;
				continue;
			}
			if (step == 1)
			{
				if (result == '\n')
				{
					step = 2;
				} 
				else
				{
					step = 0;
					data += '\r';
				}
				continue;
			}
			data += char(result);
		}
		return true;
	}

	bool yl_messenger::recv_ln_when(const String when, String &data)
	{
		String temp, when_temp(when);
		when_temp.toLowerCase();
		for (;;)
		{
			if (!recv_ln(temp))
			{
				return false;
			}
			if (temp == when)
			{
				return recv_ln(data);
			}
			for (int i=0; i<temp.length()-when.length()+1; ++i)
			{
				String sub_str = temp.substring(i, when.length() + i);
				sub_str.toLowerCase();
				if (sub_str == when)
				{
					data = temp.substring(when.length() + i);
					return true;
				}
			}
		}
		return false;
	}

	void yl_messenger::flush_stop()
	{
		flush();
		stop();
	}

	size_t yl_messenger::send(const String &data)
	{
		if (data.length() == 0)
		{
			return 0;
		}
		String temp(data);
		char *c = &temp[0];
		return write((uint8_t*)c, temp.length());
	}
}
