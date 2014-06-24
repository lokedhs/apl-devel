/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2014  Dr. Jürgen Sauermann

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <errno.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <vector>

#include "SystemLimits.hh"

#define __COMMON_HH_DEFINED__ // to avoid #error in APL_types.hh
#include "Svar_DB_memory.hh"

#include <iostream>

Svar_DB_memory db;

struct AP3_fd
{
   AP_num3 ap3;
   TCP_socket fd;
};

vector<AP3_fd> connected_procs;

const char * prog = "????";

extern ostream & get_CERR();
ostream & get_CERR() { return cerr; }

bool LOG_shared_variables = false;

//-----------------------------------------------------------------------------
static void
close_fd(TCP_socket fd)
{
   for (int j = 0; j < connected_procs.size(); ++j)
       {
         const TCP_socket fd_j = connected_procs[j].fd;
         if (fd == fd_j)
            {
              connected_procs.erase(connected_procs.begin() + j);
              break;
            }
       }

   ::close(fd);

   // exit if the last connection was removed
   //
   cerr << prog << ": closed fd " << fd << " ("
        << connected_procs.size() << " remaining)" << endl;

   if (connected_procs.size() == 0)
      {
        cerr << prog << ": exiting after last connection was closed" << endl;
        exit(0);
      }
}
//-----------------------------------------------------------------------------
static void
new_connection(TCP_socket fd)
{
   cerr << prog << ": new connection: fd " << fd << endl;

AP3_fd ap3_fd;
   ap3_fd.ap3.proc   = NO_AP;
   ap3_fd.ap3.parent = AP_NULL;
   ap3_fd.ap3.grand  = AP_NULL;
   ap3_fd.fd         = fd;

   connected_procs.push_back(ap3_fd);
}
//-----------------------------------------------------------------------------
static void
connection_readable(TCP_socket fd)
{
char command;
ssize_t len = ::recv(fd, &command, 1, 0);

   if (len <= 0)
      {
         cerr << prog << "connection[" << fd
              << "] closed (recv_TCP() returned 0" << endl;
         close_fd(fd);
         return;
      }

   switch(command)
      {
        case 'i':
             {
               cerr << "[" << fd << "] identify - ";

               // read proc/parent/grand from socket
               AP_num buffer[3];
               const ssize_t len = ::recv(fd, buffer, sizeof(buffer),
                                          MSG_WAITALL);

               if (len != sizeof(buffer))
                  {
                    cerr << prog << ": got " << len
                         << " bytes on new connection (" << sizeof(buffer)
                         << " expected) fd " << fd << endl;
                    return;
                  }

               // find connected_procs entry for fd...
               //
               int idx = -1;
               for (int j = 0; j < connected_procs.size(); ++j)
                   {
                     if (connected_procs[j].fd == fd)
                        {
                          idx = j;
                          break;
                        }
                   }

               if (idx == -1)   // not found
                  {
                    cerr << prog << ": could not find fd " << fd
                         << " in connected_procs" << endl;
                    close_fd(fd);
                    return;
                  }
               connected_procs[idx].ap3.proc   = buffer[0];
               connected_procs[idx].ap3.parent = buffer[1];
               connected_procs[idx].ap3.grand  = buffer[2];
             }
             cerr << "done." << endl;
             return;

        case 'r': cerr << "[" << fd << "] read - ";
                  len = ::send(fd, &db, sizeof(db), 0);
                  if (len != sizeof(db))
                     {
                       cerr << "*** send() failed on fd " << fd
                            << " (got " << len << " expected " << sizeof(db)
                            << "). closing fd." << endl;
                       close_fd(fd);
                       return;
                     }
                  cerr << "done." << endl;
                  return;

        case 'u': cerr << "[" << fd << "] update - ";
                  len = ::send(fd, &db, sizeof(db), 0);
                  if (len != sizeof(db))
                     {
                       cerr << "*** send() failed on fd " << fd
                            << " (got " << len << " expected " << sizeof(db)
                            << "). closing fd." << endl;
                       close_fd(fd);
                       return;
                     }

                  cerr << "sent - ";
                  // get updated db back
                  //
                  len = ::recv(fd, &db, sizeof(db), 0);
                  if (len != sizeof(db))
                     {
                       cerr << "*** recv() failed on fd " << fd
                            << " (got " << len << " expected " << sizeof(db)
                            << "). closing fd." << endl;
                       close_fd(fd);
                       return;
                     }
                  cerr << "done." << endl;
                  return;

        default: cerr << "got unknown command'" << command
                      << "' on fd " << fd << endl;
                 close_fd(fd);
                 return;
      }
}
//-----------------------------------------------------------------------------
int
main(int argc, char * argv[])
{
   prog = argv[0];

   if (argc != 2)
      {
       cerr << prog << ": missing port number" << endl;
       return 1;
      }

const int listen_port = atoi(argv[1]);
   cerr << prog << ": listening on TCP port " << listen_port << endl;

const int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
   {
     int yes = 1;
     setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
   }

sockaddr_in local;
   memset(&local, 0, sizeof(sockaddr_in));
   local.sin_family = AF_INET;
   local.sin_port = htons(listen_port);
   local.sin_addr.s_addr = htonl(0x7F000001);

   if (::bind(listen_sock, (const sockaddr *)&local, sizeof(sockaddr_in)))
      {
        cerr << prog << ": ::bind("
             << listen_port << ") failed:" << strerror(errno) << endl;
        return 3;
      }

   if (::listen(listen_sock, 10))
      {
        cerr << prog << ": ::listen(fd "
             << listen_sock << ") failed:" << strerror(errno) << endl;
        return 3;
      }

   memset(&db, 0, sizeof(db));

int max_fd = listen_sock;

   cerr << prog << ": entering main loop..." << endl;
   for (;;)
      {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(listen_sock, &read_fds);
        int max_fd = listen_sock;
        for (int j = 0; j < connected_procs.size(); ++j)
            {
              const TCP_socket fd = connected_procs[j].fd;
              if (max_fd < fd)   max_fd = fd;
              FD_SET(fd, &read_fds);
            }

        errno = 0;
        const int count = select(max_fd + 1, &read_fds, 0, 0, 0);
        if (count <= 0)
           {
             cerr << prog << ": count <= 0 in select(): "
                  << strerror(errno) << endl;
             return 3;
           }

        // something happened, check listen_sock first for new connections
        //
        if (FD_ISSET(listen_sock, &read_fds))
           {
             struct sockaddr_in from;
             socklen_t from_len = sizeof(sockaddr_in);
             const int new_fd = ::accept(listen_sock, (sockaddr *)&from,
                                         &from_len);
             if (new_fd == -1)
                {
                  cerr << prog << ": ::accept() failed: "
                       << strerror(errno) << endl;
                  exit(1);
                  continue;
                }

             new_connection((TCP_socket)new_fd);
             continue;
           }

        // connections readable
        //
        for (int j = 0; j < connected_procs.size(); ++j)
            {
              const TCP_socket fd = connected_procs[j].fd;
              if (FD_ISSET(fd, &read_fds))   connection_readable(fd);
            }
      }
}
//-----------------------------------------------------------------------------
