/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "base/weak_ptr.h"

enum class SendMediaType;

namespace Api {
struct SendAction;
struct SendOptions;
} // namespace Api

namespace HistoryView {
class ComposeControls;
} // namespace HistoryView

namespace HistoryView::Controls {
struct VoiceToSend;
} // namespace HistoryView::Controls

namespace Main {
class Session;
} // namespace Main

namespace Ui {
struct PreparedList;
class SendFilesWay;
} // namespace Ui

namespace Media::Stories {

class Controller;

struct ReplyAreaData {
	UserData *user = nullptr;
	StoryId id = 0;

	friend inline auto operator<=>(ReplyAreaData, ReplyAreaData) = default;
	friend inline bool operator==(ReplyAreaData, ReplyAreaData) = default;
};

class ReplyArea final : public base::has_weak_ptr {
public:
	explicit ReplyArea(not_null<Controller*> controller);
	~ReplyArea();

	void show(ReplyAreaData data);

	[[nodiscard]] rpl::producer<bool> focusedValue() const;

private:
	using VoiceToSend = HistoryView::Controls::VoiceToSend;

	[[nodiscard]] Main::Session &session() const;

	bool confirmSendingFiles(const QStringList &files);
	bool confirmSendingFiles(not_null<const QMimeData*> data);

	void uploadFile(const QByteArray &fileContent, SendMediaType type);
	bool confirmSendingFiles(
		QImage &&image,
		QByteArray &&content,
		std::optional<bool> overrideSendImagesAsPhotos = std::nullopt,
		const QString &insertTextOnCancel = QString());
	bool confirmSendingFiles(
		const QStringList &files,
		const QString &insertTextOnCancel);
	bool confirmSendingFiles(
		Ui::PreparedList &&list,
		const QString &insertTextOnCancel = QString());
	bool confirmSendingFiles(
		not_null<const QMimeData*> data,
		std::optional<bool> overrideSendImagesAsPhotos,
		const QString &insertTextOnCancel = QString());
	bool showSendingFilesError(const Ui::PreparedList &list) const;
	bool showSendingFilesError(
		const Ui::PreparedList &list,
		std::optional<bool> compress) const;
	void sendingFilesConfirmed(
		Ui::PreparedList &&list,
		Ui::SendFilesWay way,
		TextWithTags &&caption,
		Api::SendOptions options,
		bool ctrlShiftEnter);
	void finishSending();

	void initGeometry();
	void initActions();

	[[nodiscard]] Api::SendAction prepareSendAction(
		Api::SendOptions options) const;
	void send(Api::SendOptions options);
	void sendVoice(VoiceToSend &&data);
	void chooseAttach(std::optional<bool> overrideSendImagesAsPhotos);

	void showPremiumToast(not_null<DocumentData*> emoji);

	const not_null<Controller*> _controller;
	const std::unique_ptr<HistoryView::ComposeControls> _controls;

	ReplyAreaData _data;
	base::has_weak_ptr _shownUserGuard;
	bool _choosingAttach = false;

	rpl::lifetime _lifetime;

};

} // namespace Media::Stories