#include <Geode/Geode.hpp>
#include <Geode/modify/CustomSongLayer.hpp>
#include <matjson.hpp>
#include <fstream>

using namespace geode::prelude;

struct Song {
    int songID;
    uint64_t weight;
};

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

    std::ifstream file(path);
    if (!file.is_open()) return songs;

    auto result = matjson::parse(file);
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

		auto spr = CircleButtonSprite::create(
			CCSprite::create("overrated.png"_spr),
			CircleBaseColor::DarkAqua,
			CircleBaseSize::Big
		);
		auto button = CCMenuItemSpriteExtra::create(
			spr,
			this,
			menu_selector(MyCustomSongLayer::onOverratedSongs)
		);
		button->setPosition({ 14.0f, -190.0f });
		m_buttonMenu->addChild(button);

		return true;
	}

	void onOverratedSongs(CCObject* sender) {
		auto songs = loadSongsFromJSON((Mod::get()->getResourcesDir() / "songs.json").string());

		if (songs.empty()) {
			log::error("No valid songs found!");
			return;
		}

        int weightPercentage = Mod::get()->getSettingValue<int>("weight-percentage");
        for (auto& song : songs) {
            if (weightPercentage <= 0) {
                song.weight = 1;
            } else {
                double multiplier = static_cast<double>(weightPercentage) / 100.0;
                song.weight = static_cast<int>(std::max(1.0, static_cast<double>(song.weight) * multiplier));
            }
        }

		auto chosen = weightedChoice(songs);

		int id = chosen->songID;

		log::info("Selected Song ID: {}", id);

		m_songDelegate->songIDChanged(id);
		log::debug("Active Song ID: {}", m_songDelegate->getActiveSongID());
		m_songWidget->updateSongObject(SongInfoObject::create(id));
		m_songWidget->updateSongInfo();
		m_songWidget->getSongInfoIfUnloaded();
	}
};
