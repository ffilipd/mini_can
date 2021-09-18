/*********************************************************************************/
/* This server is a modified version of						 */
/* https://www.ibm.com/docs/en/i/7.2?topic=designs-using-poll-instead-select     */
/*********************************************************************************/

// include all neccesary libraries for this script
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
// #include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>

#define SERVER_PORT 4000 //specify the port that server is listening on

int main(int argc, char *argv[])
{
    int len, rc, on = 1;
    int listen_sd = -1, new_sd = -1;
    int desc_ready, end_server = 0, compress_array = 0;
    int close_conn;
    char buffer[80], id[10];
    struct sockaddr_in6 addr;
    int timeout;
    struct pollfd fds[200];
    int nfds = 1, current_size = 0, i, j;

    // CREATE A STREAM TO RECEIVE INCOMMING CONNECTIONS
    listen_sd = socket(AF_INET6, SOCK_STREAM, 0);
    if (listen_sd < 0)
    {
        perror("socket() failed");
        exit(-1);
    }

    // SET OPTIONS TO ALLOW SOCKET FD TO BE REUSABLE
    rc = setsockopt(listen_sd, SOL_SOCKET, SO_REUSEADDR,
                    (char *)&on, sizeof(on));
    if (rc < 0)
    {
        perror("setsockopt() failed");
        close(listen_sd);
        exit(-1);
    }

    // SET SOCKET TO BE NONBLOCKING
    // fcntl can be used instead of ioctl, in that case: include fcntl
    int status = fcntl(listen_sd, F_SETFL, fcntl(listen_sd, F_GETFL, 0) | O_NONBLOCK);

    if (status == -1)
    {
        perror("calling fcntl");
        close(listen_sd);
        exit(-1);
    }

    // INITIALIZE SOCKET ADDRESS
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    memcpy(&addr.sin6_addr, &in6addr_any, sizeof(in6addr_any));
    addr.sin6_port = htons(SERVER_PORT);

    // BIND SOCKET
    rc = bind(listen_sd, (struct sockaddr *)&addr, sizeof(addr));

    if (rc < 0)
    {
        perror("bind() failed");
        close(listen_sd);
        exit(-1);
    }

    // listen to socket with a backlog of 32.
    // backlog defines maximum queue length of pending connections.
    rc = listen(listen_sd, 32);
    if (rc < 0)
    {
        perror("listen() failed");
        close(listen_sd);
        exit(-1);
    }

    // INITIALIZE POLLFD STRUCTURE
    memset(fds, 0, sizeof(fds)); // memset sets all bytes in fds[] to "0"

    // INITIALIZE LISTENING SOCKET
    fds[0].fd = listen_sd;  // set first file descriptor to the listening socket
    fds[0].events = POLLIN; // event set to listen for incoming data

    // WAIT FOR INCOMMING CONNECTIONS
    do
    {
        // printf("Waiting on poll()...\n");
        // POLL
        // poll is set to listen to fds the argument (nfds) tells how many open fds ther is, (-1) is the timeout.
        // In this case, timeout is set to -1 which means, poll() will not timeout
        rc = poll(fds, nfds, -1);

        // POLL FAILED
        if (rc < 0)
        {
            perror("  poll() failed");
            break;
        }

        current_size = nfds;
        for (i = 0; i < current_size; i++) //loop through all open file descriptors
        {

            // FIND THE FD THAT RETURNED POLLIN AND CHECK IF LISTENING OR JUST ACTIVE
            // fds[i] has no activity
            if (fds[i].revents == 0)
                continue; // start over with next

            // fds[i] is the fd that has activity
            if (fds[i].fd == listen_sd)
            {
                // LISTENING SOCKET IS READABLE
                // printf("  Listening socket is readable\n");

                // ACCEPT ALL INCOMING CONNECTIONS IN QUEUE BEFORE POLL AGAIN
                do
                {
                    // ACCEPT EACH INCOMING CONNECTION. IF ACCEPT FAILS WITH EWOULDBLOCK -> ALL QUEUED CONNECTIONS ARE ACCEPTED
                    new_sd = accept(listen_sd, NULL, NULL);
                    if (new_sd < 0)
                    {
                        if (errno != EWOULDBLOCK)
                        {
                            perror("  accept() failed");
                            end_server = 1;
                        }
                        break;
                    }

                    // ADD NEW CONNECTION TO POLLFD STRUCTURE
                    // printf("  New incoming connection - %d\n", new_sd);
                    fds[nfds].fd = new_sd;
                    fds[nfds].events = 0;
                    fds[nfds].events = POLLIN;
                    nfds++;

                } while (new_sd != -1); // STILL CONNECTIONS TO ACCEPT
            }

            // CONNECTION IS NOT LISTENING -> CONNECTION IS READABLE
            else
            {
                // printf("  Descriptor %d is readable\n", fds[i].fd);
                close_conn = 0;

                do
                {
                    // RECEIVE DATA ON THIS SOCKET UNTIL EWOULDBLOCK OCCURS
                    // MSG_DONTWAIT is a flag set so that server continues when there is no more data to read
                    rc = recv(fds[i].fd, buffer, sizeof(buffer), MSG_DONTWAIT);
                    sprintf(id, "%d", fds[i].fd);
                    strcat(id, "/");
                    if (rc < 0)
                    {
                        if (errno != EWOULDBLOCK)
                        {
                            perror("  recv() failed");
                            close_conn = 1;
                        }
                        break;
                    }

                    // CONNECTION CLOSED
                    if (rc == 0)
                    {
                        // printf("  Connection closed\n");
                        close_conn = 1;
                        break;
                    }

                    // DATA RECEIVED
                    len = rc;

                    // printf("  %d bytes received: %s,  from client: %d\n", len, buffer, fds[i].fd);

                    // SEND DATA BACK TO CLIENT
                    // Iterate through open fds
                    for (int i = 1; i < nfds; i++)
                    {
                        // Send "buffer" to "fd[i]"", "0" is a flag that tells the socket not to try to send while server is writing
                        // printf("sending data: %s back to client: %d, length: %d\n", strcat(fds[i].fd, buffer),fds[i].fd, len);
                        strcat(id, buffer);
                        rc = send(fds[i].fd, id, strlen(id), 0);
                    }
                    if (rc < 0)
                    {
                        perror("  send() failed");
                        close_conn = 1;
                        break;
                    }

                } while (1);

                // CONNECTION WAS CLOSED
                if (close_conn)
                {
                    close(fds[i].fd); // close fd
                    fds[i].fd = -1;   // remove connection from array
                    compress_array = 1;
                }
            }
        }

        // MODIFY ARRAY AFTER CONNECTION WAS REMOVED
        if (compress_array)
        {
            compress_array = 0;
            for (i = 0; i < nfds; i++)
            {
                if (fds[i].fd == -1)
                {
                    for (j = i; j < nfds; j++)
                    {
                        fds[j].fd = fds[j + 1].fd; // from the position where closed fd was removed, shift all fds to left
                    }
                    i--;
                    nfds--; //decrease number of open fds
                }
            }
        }

    } while (end_server == 0); // Server is closing

    // Clean up, close all open fds
    for (i = 0; i < nfds; i++)
    {
        if (fds[i].fd >= 0)
            close(fds[i].fd);
    }
}
