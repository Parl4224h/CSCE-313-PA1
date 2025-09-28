/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Parker Hoffmann
	UIN: 234002242
	Date: 09/25/2025
*/
#include "common.h"
#include "FIFORequestChannel.h"


int main(int argc, char* argv[])
{
    int opt;
    int p = 1;
    double t = -1.0;
    int e = 1;
    int m = 5000;
    bool createChannel = false;
    MESSAGE_TYPE quit = QUIT_MSG;

    std::string filename = "";
    while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1)
    {
        switch (opt)
        {
        case 'p':
            p = atoi(optarg);
            break;
        case 't':
            t = atof(optarg);
            break;
        case 'e':
            e = atoi(optarg);
            break;
        case 'f':
            filename = optarg;
            break;
        case 'm':
            m = atoi(optarg);
            break;
        case 'c':
            createChannel = true;
            break;
        default:
            break;
        }
    }

    // Run client
    if (pid_t pid = fork(); pid > 0)
    {
        // Create the main channel
        FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);

        // Setup active channel and way to safely delete secondary channel
        FIFORequestChannel* active = &chan;
        FIFORequestChannel* second = nullptr;

        if (createChannel) {
            MESSAGE_TYPE newChan = NEWCHANNEL_MSG;
            chan.cwrite(&newChan, sizeof(newChan));

            char namebuf[256];
            ssize_t n = chan.cread(namebuf, sizeof(namebuf));

            // Check if response exists
            if (n <= 0) {
                std::cout << "Failed to read new channel name" << std::endl;
            } else {
                std::string channelName(namebuf);
                std::cout << "New channel: " << channelName << "\n";

                second = new FIFORequestChannel(channelName, FIFORequestChannel::CLIENT_SIDE);
                active = second;
            }
        }

        // Get file
        if (filename.size() > 0)
        {
            // Get length of file
            filemsg fm(0, 0);
            const size_t nameLen = filename.size() + 1;
            const size_t reqLen  = sizeof(filemsg) + nameLen;
            std::vector<char> req(reqLen);
            memcpy(req.data(), &fm, sizeof(filemsg));
            strcpy(req.data() + sizeof(filemsg), filename.c_str());
            active->cwrite(req.data(), reqLen);
            int64_t fileLen;
            active->cread(&fileLen, sizeof(int64_t));
            // Request file chunks
            std::ofstream fout("./received/" + filename);
            std::vector<char> data(m);
            std::strcpy(req.data() + sizeof(filemsg), filename.c_str());
            int64_t offset = 0;
            while (offset < fileLen)
            {
                // Make sure to not go over file's total size
                int chunk = static_cast<int>(std::min<int64_t>(m, fileLen - offset));

                filemsg cm(offset, chunk);
                std::memcpy(req.data(), &cm, sizeof(filemsg));

                active->cwrite(req.data(), reqLen);
                active->cread(data.data(), chunk);

                fout.write(data.data(), chunk);
                offset += chunk;
            }
        }
        // Get the first 1000 data points
        else if (t < 0)
        {
            std::vector<char> req(MAX_MESSAGE);
            std::ofstream fout("./received/x1.csv");
            for (int i = 0; i < 1000; i++)
            {
                // get current time to retreive
                fout << 0.004 * i << ',';

                // Initialize reply var
                double reply;
                size_t replySize = sizeof(double);
                size_t msgSize = sizeof(datamsg);

                // Get data point 1
                datamsg x(p, 0.004 * i, 1);
                memcpy(req.data(), &x, msgSize);
                active->cwrite(req.data(), msgSize);
                active->cread(&reply, replySize);
                fout << reply << ',';

                // Get data point 2
                datamsg y(p, 0.004 * i, 2);
                memcpy(req.data(), &y, msgSize);
                active->cwrite(req.data(), msgSize);
                active->cread(&reply, replySize);
                fout << reply << std::endl;
            }
            fout.close();
        }
        // Get a specific data point
        else
        {
            size_t msgSize = sizeof(datamsg);
            std::vector<char> req(MAX_MESSAGE);
            datamsg x(p, t, e);
            memcpy(req.data(), &x, msgSize);
            active->cwrite(req.data(), msgSize); // question
            double reply;
            active->cread(&reply, sizeof(double)); //answer
            std::cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << std::endl;
        }
        // Close channels
        if (second)
        {
            // Close the secondary channel and delete it since it was initialized using new
            active->cwrite(&quit, sizeof(quit));
            delete second;
            active = &chan;
        }
        active->cwrite(&quit, sizeof(MESSAGE_TYPE));
    }
    // Run server
    else
    {
        char mBuf[32];
        snprintf(mBuf, sizeof(mBuf), "%d", m);
        char* const args[] = {
            // Prevent compiler warnings about casting
            const_cast<char*>("./server"),
            const_cast<char*>("-m"),
            mBuf,
            nullptr
        };

        execvp(args[0], args);
    }
}
