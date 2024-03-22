/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

namespace Data {

class Birthday final {
public:
	Birthday();
	Birthday(int day, int month, int year = 0);

	[[nodiscard]] static Birthday FromSerialized(int value);
	[[nodiscard]] int serialize() const;

	[[nodiscard]] bool valid() const;

	[[nodiscard]] int day() const;
	[[nodiscard]] int month() const;
	[[nodiscard]] int year() const;

	explicit operator bool() const {
		return valid();
	}

	friend inline constexpr auto operator<=>(Birthday, Birthday) = default;
	friend inline constexpr bool operator==(Birthday, Birthday) = default;

private:
	int _value = 0;

};

} // namespace Data