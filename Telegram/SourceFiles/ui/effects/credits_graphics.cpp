/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "ui/effects/credits_graphics.h"

#include <QtCore/QDateTime>

#include "data/data_credits.h"
#include "data/data_file_origin.h"
#include "data/data_photo.h"
#include "data/data_photo_media.h"
#include "data/data_session.h"
#include "lang/lang_keys.h"
#include "main/main_session.h"
#include "ui/effects/premium_graphics.h"
#include "ui/effects/spoiler_mess.h"
#include "ui/empty_userpic.h"
#include "ui/painter.h"
#include "ui/rect.h"
#include "styles/style_credits.h"
#include "styles/style_intro.h" // introFragmentIcon.
#include "styles/style_settings.h"
#include "styles/style_dialogs.h"

#include <QtSvg/QSvgRenderer>

namespace Ui {

using PaintRoundImageCallback = Fn<void(
	Painter &p,
	int x,
	int y,
	int outerWidth,
	int size)>;

QImage GenerateStars(int height, int count) {
	constexpr auto kOutlineWidth = .6;
	constexpr auto kStrokeWidth = 3;
	constexpr auto kShift = 3;

	auto colorized = qs(Ui::Premium::ColorizedSvg(
		Ui::Premium::CreditsIconGradientStops()));
	colorized.replace(
		u"stroke=\"none\""_q,
		u"stroke=\"%1\""_q.arg(st::creditsStroke->c.name()));
	colorized.replace(
		u"stroke-width=\"1\""_q,
		u"stroke-width=\"%1\""_q.arg(kStrokeWidth));
	auto svg = QSvgRenderer(colorized.toUtf8());
	svg.setViewBox(svg.viewBox() + Margins(kStrokeWidth));

	const auto starSize = Size(height - kOutlineWidth * 2);

	auto frame = QImage(
		QSize(
			(height + kShift * (count - 1)) * style::DevicePixelRatio(),
			height * style::DevicePixelRatio()),
		QImage::Format_ARGB32_Premultiplied);
	frame.setDevicePixelRatio(style::DevicePixelRatio());
	frame.fill(Qt::transparent);
	const auto drawSingle = [&](QPainter &q) {
		const auto s = kOutlineWidth;
		q.save();
		q.translate(s, s);
		q.setCompositionMode(QPainter::CompositionMode_Clear);
		svg.render(&q, QRectF(QPointF(s, 0), starSize));
		svg.render(&q, QRectF(QPointF(s, s), starSize));
		svg.render(&q, QRectF(QPointF(0, s), starSize));
		svg.render(&q, QRectF(QPointF(-s, s), starSize));
		svg.render(&q, QRectF(QPointF(-s, 0), starSize));
		svg.render(&q, QRectF(QPointF(-s, -s), starSize));
		svg.render(&q, QRectF(QPointF(0, -s), starSize));
		svg.render(&q, QRectF(QPointF(s, -s), starSize));
		q.setCompositionMode(QPainter::CompositionMode_SourceOver);
		svg.render(&q, Rect(starSize));
		q.restore();
	};
	{
		auto q = QPainter(&frame);
		q.translate(frame.width() / style::DevicePixelRatio() - height, 0);
		for (auto i = count; i > 0; --i) {
			drawSingle(q);
			q.translate(-kShift, 0);
		}
	}
	return frame;
}

not_null<Ui::RpWidget*> CreateSingleStarWidget(
		not_null<Ui::RpWidget*> parent,
		int height) {
	const auto widget = Ui::CreateChild<Ui::RpWidget>(parent);
	const auto image = GenerateStars(height, 1);
	widget->resize(image.size() / style::DevicePixelRatio());
	widget->paintRequest(
	) | rpl::start_with_next([=] {
		auto p = QPainter(widget);
		p.drawImage(0, 0, image);
	}, widget->lifetime());
	widget->setAttribute(Qt::WA_TransparentForMouseEvents);
	return widget;
}

PaintRoundImageCallback GenerateCreditsPaintUserpicCallback(
		const Data::CreditsHistoryEntry &entry) {
	const auto bg = [&]() -> Ui::EmptyUserpic::BgColors {
		switch (entry.peerType) {
		case Data::CreditsHistoryEntry::PeerType::Peer:
			return Ui::EmptyUserpic::UserpicColor(0);
		case Data::CreditsHistoryEntry::PeerType::AppStore:
			return { st::historyPeer7UserpicBg, st::historyPeer7UserpicBg2 };
		case Data::CreditsHistoryEntry::PeerType::PlayMarket:
			return { st::historyPeer2UserpicBg, st::historyPeer2UserpicBg2 };
		case Data::CreditsHistoryEntry::PeerType::Fragment:
			return { st::historyPeer8UserpicBg, st::historyPeer8UserpicBg2 };
		case Data::CreditsHistoryEntry::PeerType::PremiumBot:
			return { st::historyPeer8UserpicBg, st::historyPeer8UserpicBg2 };
		case Data::CreditsHistoryEntry::PeerType::Unsupported:
			return {
				st::historyPeerArchiveUserpicBg,
				st::historyPeerArchiveUserpicBg,
			};
		}
		Unexpected("Unknown peer type.");
	}();
	const auto userpic = std::make_shared<Ui::EmptyUserpic>(bg, QString());
	return [=](Painter &p, int x, int y, int outerWidth, int size) mutable {
		userpic->paintCircle(p, x, y, outerWidth, size);
		using PeerType = Data::CreditsHistoryEntry::PeerType;
		if (entry.peerType == PeerType::PremiumBot) {
			return;
		}
		const auto rect = QRect(x, y, size, size);
		((entry.peerType == PeerType::AppStore)
			? st::sessionIconiPhone
			: (entry.peerType == PeerType::PlayMarket)
			? st::sessionIconAndroid
			: (entry.peerType == PeerType::Fragment)
			? st::introFragmentIcon
			: st::dialogsInaccessibleUserpic).paintInCenter(p, rect);
	};
}

Fn<void(Painter &, int, int, int, int)> GenerateCreditsPaintEntryCallback(
		not_null<PhotoData*> photo,
		Fn<void()> update) {
	struct State {
		std::shared_ptr<Data::PhotoMedia> view;
		Image *imagePtr = nullptr;
		QImage image;
		rpl::lifetime downloadLifetime;
		bool entryImageLoaded = false;
	};
	const auto state = std::make_shared<State>();
	state->view = photo->createMediaView();
	photo->load(Data::PhotoSize::Thumbnail, {});

	rpl::single(rpl::empty_value()) | rpl::then(
		photo->owner().session().downloaderTaskFinished()
	) | rpl::start_with_next([=] {
		using Size = Data::PhotoSize;
		if (const auto large = state->view->image(Size::Large)) {
			state->imagePtr = large;
		} else if (const auto small = state->view->image(Size::Small)) {
			state->imagePtr = small;
		} else if (const auto t = state->view->image(Size::Thumbnail)) {
			state->imagePtr = t;
		}
		update();
		if (state->view->loaded()) {
			state->entryImageLoaded = true;
			state->downloadLifetime.destroy();
		}
	}, state->downloadLifetime);

	return [=](Painter &p, int x, int y, int outerWidth, int size) {
		if (state->imagePtr
			&& (!state->entryImageLoaded || state->image.isNull())) {
			const auto image = state->imagePtr->original();
			const auto minSize = std::min(image.width(), image.height());
			state->image = Images::Prepare(
				image.copy(
					(image.width() - minSize) / 2,
					(image.height() - minSize) / 2,
					minSize,
					minSize),
				size * style::DevicePixelRatio(),
				{ .options = Images::Option::RoundCircle });
		}
		p.drawImage(x, y, state->image);
	};
}

Fn<void(Painter &, int, int, int, int)> GeneratePaidMediaPaintCallback(
		not_null<PhotoData*> photo,
		Fn<void()> update) {
	struct State {
		explicit State(Fn<void()> update) : spoiler(std::move(update)) {
		}

		QImage image;
		Ui::SpoilerAnimation spoiler;
	};
	const auto state = std::make_shared<State>(update);

	return [=](Painter &p, int x, int y, int outerWidth, int size) {
		if (state->image.isNull()) {
			const auto media = photo->createMediaView();
			const auto thumbnail = media->thumbnailInline();
			const auto ratio = style::DevicePixelRatio();
			const auto scaled = QSize(size, size) * ratio;
			auto image = thumbnail
				? Images::Blur(thumbnail->original(), true)
				: QImage(scaled, QImage::Format_ARGB32_Premultiplied);
			if (!thumbnail) {
				image.fill(Qt::black);
				image.setDevicePixelRatio(ratio);
			}
			const auto minSize = std::min(image.width(), image.height());
			state->image = Images::Prepare(
				image.copy(
					(image.width() - minSize) / 2,
					(image.height() - minSize) / 2,
					minSize,
					minSize),
				size * ratio,
				{ .options = Images::Option::RoundLarge });
		}
		p.drawImage(x, y, state->image);
		Ui::FillSpoilerRect(
			p,
			QRect(x, y, size, size),
			Ui::DefaultImageSpoiler().frame(
				state->spoiler.index(crl::now(), false)));
	};
}

TextWithEntities GenerateEntryName(const Data::CreditsHistoryEntry &entry) {
	return ((entry.peerType == Data::CreditsHistoryEntry::PeerType::Fragment)
		? tr::lng_bot_username_description1_link
		: tr::lng_credits_summary_history_entry_inner_in)(
			tr::now,
			TextWithEntities::Simple);
}

} // namespace Ui