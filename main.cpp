/*
This program aims to achieve a single-thread implementation of the Go-Back-N ARQ protocol in C++.
*/

#include <bits/stdc++.h>

using namespace std;
using namespace std::chrono; // For milliseconds() function in chrono

// Function to simulate a random event given a probability (between 0.0 to 1.0)
bool chance(double probability, mt19937 &rng) {

    // uniform_real_distribution is a class in the random library.
    // Here, we create an instance of the class - dist, and pass the range of the uniform distribution into the constructor.
    uniform_real_distribution<double> dist(0.0, 1.0);

    // dist(rng) returns a random number in range 0.0-1.0.
    // If random number is less than the probability, then the function returns true, simulating a successful transmission.
    return dist(rng) < probability;
}

int main() {

    // Simulation parameters:
    const int totalPackets = 10;        // Total number of packets to send
    const int senderWindowSize = 3;     // Sender window size; (2^m) - 1, m=2
    const double dataSuccessProb = 0.8;   // 80% chance for successful data transmission
    const double ackSuccessProb = 0.5;    // 50% chance for successful ACK transmission
    const int timeoutDurationMs = 2000;   // Timeout duration in milliseconds

    // Random number generator setup:

    // Creating an object rd from the class random_device. This acts as a seed for mt19937, a puedo-RNG.
    // We need to create a random seed. Using a fixed seed value, the puesdo-RNG would always generate same sequence of numbers.
    random_device rd;

    // Creating an object rng from class Mersenne Twister 19937. This is a Pseudo-Random Number Generator.
    // We pass rd as the seed to this PRNG engine.
    mt19937 rng(rd());

    int senderBase = 0; // First unacknowledged packet at sender
    int receiverExpected = 0; // This is receiver's expected packet number

    while (senderBase < totalPackets) { // loop runs till all packets have been acknowledged
        // This runs whenever there is a change in the sender's window.

        bool ackForBaseReceived = false; // Intializing variable for whether ACK received for base of window
        cout << "\n--- Sender's Window: Packets " << senderBase 
             << " to " << min(senderBase + senderWindowSize - 1, totalPackets - 1) << " ---\n";

        // Send packets in the current window one-by-one
        for (int seq = senderBase; seq < senderBase + senderWindowSize && seq < totalPackets; ++seq) {
            cout << "[Sender] Sending packet " << seq << endl;
            
            // Simulate transmission of data (80% chance of success)
            if (chance(dataSuccessProb, rng)) {
                // This runs when sender's data is successfully received at the receiver.

                // At the receiver side:
                if (seq == receiverExpected) {
                    // This runs if the packet received is within the receiver's window

                    cout << "[Receiver] Received expected packet " << seq << endl;
                    cout << "[Receiver] Sliding window from packet " << receiverExpected 
                         << " to packet " << receiverExpected + 1 << endl;
                    if (receiverExpected + 1 < totalPackets) {
                        receiverExpected++; // Receiver immediately slides its window
                        cout << "[Receiver] Sending ACK for packet " << receiverExpected << endl;
                        // Receiver sends ACK for next expected packet
                    }
                    
                    // Simulate ACK transmission (50% chance of success)
                    if (chance(ackSuccessProb, rng)) {
                        // This runs when the ACK is successfully received at the sender
                        cout << "[Sender] Received ACK for packet " << receiverExpected << endl;
                        
                        // Since this is the base packet, slide sender's window immediately:
                        senderBase = seq + 1;
                        ackForBaseReceived = true;
                        
                        // Immediately break out of the loop to start transmission with the new window:
                        break;
                    } else {
                        cout << "[Receiver] ACK for packet " << receiverExpected << " lost" << endl;
                    }

                } else {
                    // This runs if the packet received is outside the receiver's window

                    // Discarding the packet:
                    cout << "[Receiver] Received out-of-order packet " << seq 
                         << " (expected " << receiverExpected << ") - discarded" << endl;
                    
                    /*There is a bug in Go-Back-N ARQ:
                    Suppose the sender's window is 1-3.
                    Now suppose sender successfully transmits packet 3 and that is what the receiver expects,
                    so it shifts its window to 4. However ACK 3 gets lost in transmission.
                    Sender will keep infinitely sending packets 1-3 but none of them will be received by the receiver
                    as they are outside its window (4).
                    To solve this bug, we need to resend ACK for the last successfully received packet so that 
                    sender's window can be updated. We just repeat the above ACK code for receiverExpected - 1.*/

                    cout << "[Receiver] Sending ACK for packet " << receiverExpected << endl; // Receiver sends ACK from its side
                    if (chance(ackSuccessProb, rng)) {
                        cout << "[Sender] Received ACK for packet " << receiverExpected << endl;
                        senderBase = receiverExpected;
                        ackForBaseReceived = true;
                        break;
                    } else {
                        cout << "[Receiver] ACK for packet " << receiverExpected << " lost" << endl;
                    }
                }

            } else {
                // This runs when the sender's data is not received at the receiver.
                cout << "[Sender] Packet " << seq << " lost during transmission" << endl;
            }
        }

        // If no ACK for the base packet was received during the window transmission, timeout and retransmit:
        if (!ackForBaseReceived) {
            cout << "[Sender] Timeout for packet " << senderBase 
                 << ". Retransmitting window." << endl;
            this_thread::sleep_for(milliseconds(timeoutDurationMs));
        } 
        
        // Adding a small delay before proceeding with the new window:
        this_thread::sleep_for(milliseconds(500));
        
    }

    cout << "\n[Sender] All packets have been successfully sent and acknowledged." << endl;
    return 0;
}
