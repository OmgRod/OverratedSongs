#include <Geode/Geode.hpp>
#include <Geode/modify/CustomSongLayer.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/utils/file.hpp>
#include <matjson.hpp>

using namespace geode::prelude;
struct Song {
    int songID;
    uint64_t weight;
};

static async::TaskHolder<web::WebResponse> s_refreshTask;

std::optional<Song> weightedChoice(const std::vector<Song>& songs) {
    uint64_t totalWeight = 0;

    for (const auto& song : songs)
        totalWeight += song.weight;

    if (totalWeight == 0) {
		FLAlertLayer::create(
			"Error",
			"There were no songs available to choose from. If you see this, <cr>please report this to the developer</c>.",
			"OK"
		)->show();

		return std::nullopt;
	}

    uint64_t roll = geode::utils::random::generate<uint64_t>(0, totalWeight);

    uint64_t current = 0;

    for (const auto& song : songs) {
        current += song.weight;

        if (roll < current)
            return song;
    }

    return songs.back();
}

std::vector<Song> loadSongsFromJSON(const std::string& path) {
    std::vector<Song> songs;

    auto result = file::readJson(path);
    if (!result) {
        log::error("Failed to parse songs.json: {}", result.unwrapErr());
        return songs;
    }

    auto root = result.unwrap();
    if (!root.isArray()) return songs;

    for (auto& element : root.asArray().unwrap()) {
        if (!element.isObject()) continue;

        auto songIDRes = element["songID"].asInt();
        auto freqRes = element["frequency"].asInt();

        if (songIDRes && freqRes) {
            songs.push_back({
                static_cast<int>(songIDRes.unwrap()),
                static_cast<uint64_t>(freqRes.unwrap())
            });
        }
    }

    return songs;
}

class $modify(MyCustomSongLayer, CustomSongLayer) {
	bool init(CustomSongDelegate* delegate) {
		if (!CustomSongLayer::init(delegate)) return false;

		auto rngSpr = CircleButtonSprite::create(
			CCSprite::create("overrated.png"_spr),
			CircleBaseColor::DarkAqua,
			CircleBaseSize::Big
		);
		auto rngButton = CCMenuItemSpriteExtra::create(
			rngSpr,
			this,
			menu_selector(MyCustomSongLayer::onOverratedSongs)
		);
		rngButton->setPosition({ 14.0f, -190.0f });
		m_buttonMenu->addChild(rngButton);

		auto refreshSpr = CircleButtonSprite::create(
			CCSprite::create("refresh.png"_spr),
			CircleBaseColor::DarkAqua,
			CircleBaseSize::Small
		);
		auto refreshButton = CCMenuItemSpriteExtra::create(
			refreshSpr,
			this,
			menu_selector(MyCustomSongLayer::onRefreshSongs)
		);
		refreshButton->setPosition({ 14.0f, -140.0f });
		m_buttonMenu->addChild(refreshButton);

		return true;
	}

	void onOverratedSongs(CCObject* sender) {
		auto songs = loadSongsFromJSON(geode::utils::string::pathToString(Mod::get()->getResourcesDir() / "songs.json"));

		if (songs.empty()) {
			log::error("No valid songs found!");
			return;
		}

        int64_t weightPercentage = Mod::get()->getSettingValue<int64_t>("weight-percentage");
        for (auto& song : songs) {
            if (weightPercentage <= 0) {
                song.weight = 1;
            } else {
                double multiplier = static_cast<double>(weightPercentage) / 100.0;
                song.weight = static_cast<uint64_t>(std::max(1.0, static_cast<double>(song.weight) * multiplier));
            }
        }

		auto chosen = weightedChoice(songs);

		int id = chosen->songID;

		log::info("Selected Song ID: {}", id);

		m_songWidget->updateSongObject(SongInfoObject::create(id));
		m_songWidget->updateSongInfo();
		m_songWidget->getSongInfoIfUnloaded();
	}

	void onRefreshSongs(CCObject* sender) {
        auto req = web::WebRequest();
        
        s_refreshTask.spawn(
            "Refreshing Songs",
            req.get("https://raw.githubusercontent.com/OmgRod/OverratedSongs/refs/heads/master/res/files/songs.json"),
            [this](web::WebResponse res) {
                if (!res.ok()) {
                    Notification::create("Failed to fetch updated songs list.", NotificationIcon::Error)->show();
                    return;
                }

                auto newJsonRes = res.json();
                if (!newJsonRes) {
                    Notification::create("Received invalid data from server. Please try again later.", NotificationIcon::Error)->show();
                    return;
                }

                auto newJson = newJsonRes.unwrap();
                auto currentPath = Mod::get()->getResourcesDir() / "songs.json";
                
                auto currentJsonRes = file::readJson(currentPath);
                
                if (currentJsonRes && currentJsonRes.unwrap() == newJson) {
                    Notification::create("Songs list is already up to date.", NotificationIcon::Info)->show();
                    return;
                }

                auto writeRes = file::writeString(currentPath, newJson.dump(4));
                if (!writeRes) {
                    Notification::create("Failed to save updated songs list.", NotificationIcon::Error)->show();
                    return;
                }

                Notification::create("Songs list updated successfully!", NotificationIcon::Success)->show();
            }
        );
	}
};
