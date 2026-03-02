// g++ -o main main.cpp -I/usr/local/include/aubio -L/usr/local/lib -laubio

#include <iostream>
#include <string>
#include <vector>

#include <aubio/aubio.h>

class BeatDetector
{
  private:
    uint_t sample_rate;
    uint_t hop_size;
    uint_t win_size;

  public:
    BeatDetector(uint_t sr = 44100, uint_t hop = 512, uint_t win = 1024) : sample_rate(sr), hop_size(hop), win_size(win)
    {
    }

    std::vector<double> detectBeats(const std::string &filename)
    {
        std::vector<double> beats;

        // Create source
        aubio_source_t *source = new_aubio_source(filename.c_str(), sample_rate, hop_size);
        if (!source)
        {
            std::cerr << "Error: could not open " << filename << std::endl;
            return beats;
        }

        // Create tempo detection object
        aubio_tempo_t *tempo = new_aubio_tempo("default", win_size, hop_size, sample_rate);

        // Allocate memory
        fvec_t *in = new_fvec(hop_size);
        fvec_t *out = new_fvec(1);

        uint_t read = 0;
        uint_t total_frames = 0;

        do
        {
            // Read audio file
            aubio_source_do(source, in, &read);

            // Execute tempo detection
            aubio_tempo_do(tempo, in, out);

            // Check if beat
            if (out->data[0] != 0)
            {
                double beat_time = aubio_tempo_get_last_s(tempo);
                beats.push_back(beat_time);
            }

            total_frames += read;
        } while (read == hop_size);

        // Clean up
        del_aubio_tempo(tempo);
        del_aubio_source(source);
        del_fvec(in);
        del_fvec(out);

        return beats;
    }
};

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <audiofile>" << std::endl;
        return 1;
    }

    // Initialize aubio library
    // aubio_init();

    BeatDetector detector;
    std::vector<double> beats = detector.detectBeats(argv[1]);

    // Print beat times
    for (double beat_time : beats)
    {
        std::cout << beat_time << std::endl;
    }

    // Clean up aubio
    aubio_cleanup();

    return 0;
}
