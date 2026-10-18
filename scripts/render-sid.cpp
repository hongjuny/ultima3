/* SPDX-License-Identifier: GPL-2.0-or-later
 * Offline soundtrack reproduction tool. This executable is not part of the app.
 * Links libsidplayfp/reSIDfp; see scripts/sid-renderer-COPYING.txt.
 */
#include "sidplayfp/sidplayfp.h"
#include "sidplayfp/SidConfig.h"
#include "sidplayfp/SidTune.h"
#include "sidplayfp/SidTuneInfo.h"
#include "sidplayfp/SidInfo.h"
#include "builders/residfp-builder/residfp.h"
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

static uint32_t integer(const char *text, uint32_t maximum) {
    const std::string value(text);
    if (value.empty() || value.find_first_not_of("0123456789") != std::string::npos)
        throw std::runtime_error("Expected a positive decimal integer");
    const auto parsed = std::stoull(value);
    if (!parsed || parsed > maximum) throw std::runtime_error("Argument outside supported range");
    return static_cast<uint32_t>(parsed);
}

static void littleEndian(std::ostream &out, uint32_t value, unsigned bytes) {
    for (unsigned i = 0; i < bytes; ++i) out.put(static_cast<char>(value >> (8 * i)));
}

int main(int argc, char **argv) {
    if (argc != 5) {
        std::cerr << "Usage: render-sid INPUT.sid SUBTUNE OUTPUT.wav FRAME_COUNT\n"
                     "Writes mono 44100 Hz signed 16-bit PCM; maximum 20 minutes.\n";
        return 2;
    }
    std::filesystem::path temporary;
    bool ownsTemporary = false;
    try {
        static_assert(sizeof(short) == 2, "libsidplayfp samples must be 16-bit");
        const uint32_t song = integer(argv[2], 10);
        const uint32_t frames = integer(argv[4], 44100 * 60 * 20);
        const std::filesystem::path input(argv[1]), output(argv[3]);
        if (std::filesystem::weakly_canonical(input) == std::filesystem::weakly_canonical(output))
            throw std::runtime_error("Input and output must be different files");
        temporary = output;
        temporary += ".part";
        if (std::filesystem::exists(temporary))
            throw std::runtime_error("Output .part file already exists");

        SidTune tune(input.c_str());
        if (!tune.getStatus()) throw std::runtime_error(tune.statusString());
        const auto info = tune.getInfo();
        if (!info || info->songs() != 10 || info->sidChips() != 1 ||
            info->clockSpeed() != SidTuneInfo::CLOCK_NTSC ||
            info->sidModel(0) != SidTuneInfo::SIDMODEL_6581 ||
            tune.selectSong(song) != song)
            throw std::runtime_error("Expected the ten-subtune NTSC MOS6581 Ultima III SID source");

        ReSIDfpBuilder builder("reSIDfp");
        if (builder.create(1) != 1) throw std::runtime_error(builder.error());
        builder.filter(true);
        builder.filter6581Curve(0.5);
        builder.filter6581Range(0.5);
        builder.combinedWaveformsStrength(SidConfig::AVERAGE);
        sidplayfp player;
        if (std::string(player.info().version()) != "2.12.0")
            throw std::runtime_error("Canonical rendering requires libsidplayfp 2.12.0");
        SidConfig config = player.config();
        config.frequency = 44100;
        config.playback = SidConfig::MONO;
        config.defaultC64Model = SidConfig::NTSC;
        config.forceC64Model = false;
        config.defaultSidModel = SidConfig::MOS6581;
        config.forceSidModel = false;
        config.digiBoost = false;
        config.ciaModel = SidConfig::MOS6526;
        config.secondSidAddress = config.thirdSidAddress = 0;
        config.leftVolume = config.rightVolume = 1024;
        config.powerOnDelay = 0;
        config.samplingMethod = SidConfig::RESAMPLE_INTERPOLATE;
        config.fastSampling = false;
        config.sidEmulation = &builder;
        if (!player.config(config) || !player.load(&tune)) throw std::runtime_error(player.error());

        std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
        if (!out) throw std::runtime_error("Cannot create output file");
        ownsTemporary = true;
        out.exceptions(std::ios::failbit | std::ios::badbit);
        out.write("RIFF", 4); littleEndian(out, 36 + frames * 2, 4);
        out.write("WAVEfmt ", 8); littleEndian(out, 16, 4);
        littleEndian(out, 1, 2); littleEndian(out, 1, 2);
        littleEndian(out, 44100, 4); littleEndian(out, 88200, 4);
        littleEndian(out, 2, 2); littleEndian(out, 16, 2);
        out.write("data", 4); littleEndian(out, frames * 2, 4);
        // Preserve the canonical 10 ms request size, including the final partial block.
        std::vector<short> samples(441);
        for (uint32_t position = 0; position < frames;) {
            const uint32_t count = std::min<uint32_t>(samples.size(), frames - position);
            if (player.play(samples.data(), count) != count) throw std::runtime_error(player.error());
            for (uint32_t i = 0; i < count; ++i)
                littleEndian(out, static_cast<uint16_t>(samples[i]), 2);
            position += count;
        }
        out.close();
        player.stop();
        std::filesystem::rename(temporary, output);
        temporary.clear();
        std::cerr << "libsidplayfp 2.12.0/reSIDfp: NTSC MOS6581, subtune " << song
                  << ", " << frames << " mono 44100 Hz PCM16 frames; no postprocessing\n";
        return 0;
    } catch (const std::exception &error) {
        if (ownsTemporary && !temporary.empty()) {
            std::error_code ignored;
            std::filesystem::remove(temporary, ignored);
        }
        std::cerr << "render-sid: " << error.what() << '\n';
        return 1;
    }
}
