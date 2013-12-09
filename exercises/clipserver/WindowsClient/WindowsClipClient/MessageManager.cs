using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Net;
using System.Net.Sockets;
using System.Web.Script.Serialization;
using System.Threading;

namespace ClipClient
{
    enum MessageType
    {
        HEADER = 100,
        AUTHENTICATE_REQUEST,
        AUTHENTICATE_RESPONSE,
        REFRESH_INDEX_REQUEST,
        REFRESH_INDEX_RESPONSE,
        NOTIFY_AVAILABLE_REQUEST,
        SYNC_REQUEST,
        SYNC_RESPONSE,
        PING,
    }

    enum DatasetType
    {
        NONE,
        CLIPBOARD,
        NUM_DATASETS
    }

    public class MessageManager
    {
        private class MessageHeader
        {
            public Int16 payload_size;
        }

        private class AuthenticateMessage
        {
            public UInt16 message_id = (UInt16)MessageType.AUTHENTICATE_REQUEST;
            public UInt64 user_id;
            public UInt64 device_id;
            public UInt64 secret;
        }

        private class NotifyMessage
        {
            public UInt16 message_id = (UInt16)MessageType.NOTIFY_AVAILABLE_REQUEST;
            public UInt64 token;
            public UInt16 dataset_id;
            public UInt64 content_id;
            public List<String> content_type;
            public String description;
        }

        private class SyncMessage
        {
            public UInt16 message_id = (UInt16)MessageType.SYNC_REQUEST;
            public UInt64 token;
            public UInt64 content_id;
            public UInt16 dataset_id;
        }

        private class SyncResponseMessage
        {
            public UInt16 message_id = (UInt16)MessageType.SYNC_RESPONSE;
            public UInt64 token;
            public UInt64 content_id;
            public UInt32 content_size;
            public UInt16 dataset_id;
            public UInt32 message;
        }

        private class PingMessage
        {
            public UInt16 message_id = (UInt16)MessageType.PING;
            public UInt64 token;
        }

        private Dictionary<UInt16, List<Action<Dictionary<String, Object>>>> receiver = new Dictionary<UInt16, List<Action<Dictionary<String, Object>>>>();
        private Dictionary<UInt16, Queue<Action<Dictionary<String, Object>>>> rpchandler = new Dictionary<UInt16, Queue<Action<Dictionary<String, Object>>>>();
        private Dictionary<UInt64, String> contentCache = new Dictionary<ulong, string>();

        private IPEndPoint endpoint = null;
        private Socket socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
        private SocketAsyncEventArgs recvev = new SocketAsyncEventArgs();
        private SocketAsyncEventArgs sendev = new SocketAsyncEventArgs();
        private SocketAsyncEventArgs connectev = new SocketAsyncEventArgs();
        private AutoResetEvent recvsignal = new AutoResetEvent(false);
        private AutoResetEvent sendsignal = new AutoResetEvent(false);
        private AutoResetEvent connectsignal = new AutoResetEvent(false);

        private JavaScriptSerializer serializer = new JavaScriptSerializer();

        private const int SEND_BUFFER_SIZE = 8192;
        private const int RECV_BUFFER_SIZE = 8192;
        private const int MSG_BUFFER_SIZE = 2048;
        private byte[] buffer = new byte[SEND_BUFFER_SIZE + RECV_BUFFER_SIZE];
        private byte[] msgbuf = new byte[MSG_BUFFER_SIZE];
        private byte[] sendbuf, recvbuf;
        private int sendbytes, recvbytes, recvtotal, recvcopied, recvremaining;

        private const int PING_TIMEOUT_MS = 3000;
        private Task recvtask = null;

        private UInt64 token = 0;

        public MessageManager()
        {
            sendev.Completed += SendCompleted;
            recvev.Completed += RecvCompleted;
            connectev.Completed += ConnectCompleted;

            rpchandler.Add((UInt16)MessageType.AUTHENTICATE_REQUEST, new Queue<Action<Dictionary<String, Object>>>());
            rpchandler.Add((UInt16)MessageType.AUTHENTICATE_RESPONSE, new Queue<Action<Dictionary<String, Object>>>());
            rpchandler.Add((UInt16)MessageType.REFRESH_INDEX_REQUEST, new Queue<Action<Dictionary<String, Object>>>());
            rpchandler.Add((UInt16)MessageType.REFRESH_INDEX_RESPONSE, new Queue<Action<Dictionary<String, Object>>>());
            rpchandler.Add((UInt16)MessageType.NOTIFY_AVAILABLE_REQUEST, new Queue<Action<Dictionary<String, Object>>>());
            rpchandler.Add((UInt16)MessageType.SYNC_REQUEST, new Queue<Action<Dictionary<String, Object>>>());
            rpchandler.Add((UInt16)MessageType.SYNC_RESPONSE, new Queue<Action<Dictionary<String, Object>>>());

            receiver.Add((UInt16)MessageType.AUTHENTICATE_REQUEST, new List<Action<Dictionary<String, Object>>>());
            receiver.Add((UInt16)MessageType.AUTHENTICATE_RESPONSE, new List<Action<Dictionary<String, Object>>>());
            receiver.Add((UInt16)MessageType.REFRESH_INDEX_REQUEST, new List<Action<Dictionary<String, Object>>>());
            receiver.Add((UInt16)MessageType.REFRESH_INDEX_RESPONSE, new List<Action<Dictionary<String, Object>>>());
            receiver.Add((UInt16)MessageType.NOTIFY_AVAILABLE_REQUEST, new List<Action<Dictionary<String, Object>>>());
            receiver.Add((UInt16)MessageType.SYNC_REQUEST, new List<Action<Dictionary<String, Object>>>());
            receiver.Add((UInt16)MessageType.SYNC_RESPONSE, new List<Action<Dictionary<String, Object>>>());
            receiver.Add((UInt16)MessageType.PING, new List<Action<Dictionary<String, Object>>>());

            receiver[(UInt16)MessageType.NOTIFY_AVAILABLE_REQUEST].Add(dictionary =>
                {
                    OnContentAvailable(new NotifyEventArgs()
                    {
                        ContentId = Convert.ToUInt64(dictionary["content_id"]),
                        ContentType = new List<String>(Array.ConvertAll((Object[])dictionary["content_type"], Convert.ToString)),
                        Description = (String)dictionary["description"]
                    });
                });

            receiver[(UInt16)MessageType.SYNC_REQUEST].Add(dictionary =>
            {
                var content_id = Convert.ToUInt64(dictionary["content_id"]);
                String content = null;
                UInt32 content_size = 0;
                UInt32 message = 0;

                lock (contentCache)
                {
                    if (contentCache.ContainsKey(content_id))
                    {
                        content = contentCache[content_id];
                        content_size = (UInt32)content.Length;
                        message = 1;
                    }
                }

                var msg = new SyncResponseMessage
                {
                    token = token,
                    content_id = content_id,
                    content_size = content_size,
                    dataset_id = (UInt16)DatasetType.CLIPBOARD,
                    message = message
                };

                Send(serializer.Serialize(msg)).ContinueWith(msgtask =>
                    {
                        if (msgtask.Result)
                            SendContent(content).ContinueWith(contask =>
                            {
                                if (contask.Result == false)
                                    Console.WriteLine("failed to send sync content.");
                            });
                        else Console.WriteLine("failed to send sync response.");
                    });
            });

            Disconnected += (sender, ev) =>
                {
                    try
                    {
                        socket.Disconnect(true);
                    }
                    catch (SocketException ex) {} // will throw if we're not connected, which is fine
                };
        }

        public class DisconnectEventArgs : EventArgs
        {
            public SocketError Error;
        }

        public event EventHandler<DisconnectEventArgs> Disconnected;

        protected void OnDisconnected(DisconnectEventArgs e)
        {
            if (Disconnected != null)
                Disconnected(this, e);
        }

        public SocketError LastError { get; private set; }

        public bool IsConnected
        {
            get { return socket != null && socket.Connected; }
        }

        public Task<bool> Connect(IPAddress address, Int32 port)
        {
            return Connect(new IPEndPoint(address, port));
        }

        public Task<bool> Connect(IPEndPoint endpoint)
        {
            recvev.RemoteEndPoint =
            sendev.RemoteEndPoint =
            connectev.RemoteEndPoint =
            this.endpoint = endpoint;

            return Task.Factory.StartNew(() =>
                {
                    if (socket.ConnectAsync(connectev))
                        connectsignal.WaitOne();
                    else ConnectCompleted(this, connectev);
                }).ContinueWith(task =>
                    {
                        if (IsConnected) Task.Factory.StartNew(() =>
                        {
                            Console.WriteLine("------ listening for incoming messages");

                            var header = new MessageHeader();
                            while (IsConnected && RecvHeader(msgbuf, header) && Recv(msgbuf, header.payload_size))
                            {
                                var str = Encoding.UTF8.GetString(msgbuf, 0, header.payload_size);
                                var dictionary = serializer.DeserializeObject(str) as Dictionary<String, Object>;
                                var message_id = Convert.ToUInt16((Int32)dictionary["message_id"]);
                                Console.WriteLine("received message: " + message_id);

                                lock (receiver)
                                {
                                    if (receiver.ContainsKey(message_id))
                                    {
                                        var handlers = receiver[message_id];
                                        foreach (var handler in handlers)
                                            handler(dictionary);
                                    }
                                }

                                lock (rpchandler)
                                {
                                    if (rpchandler.ContainsKey(message_id))
                                    {
                                        var handlers = rpchandler[message_id];
                                        if (handlers.Count > 0)
                                        {
                                            var handler = handlers.Dequeue();
                                            handler(dictionary);
                                        }
                                    }
                                }
                            }
                        });
                        else Console.WriteLine("unable to connect to server");

                        return IsConnected;

                    }).ContinueWith(task =>
                    {
                        if (task.Result)
                        {
                            Action<Task> ping = null;
                            ping = t =>
                                {
                                    Console.WriteLine("pinging...");
                                    SocketError error;

                                    try
                                    {
                                        var message = new PingMessage() { token = token };
                                        var buffer = Encoding.UTF8.GetBytes(serializer.Serialize(message));
                                        var header = new MessageHeader() { payload_size = (Int16)buffer.Length };
                                        var size = BitConverter.GetBytes(IPAddress.HostToNetworkOrder(header.payload_size));
                                        error = socket.Send(size) == sizeof(Int16) && socket.Send(buffer) == buffer.Length? 
                                            SocketError.Success :
                                            SocketError.SocketError;
                                    }
                                    catch (SocketException e)
                                    {
                                        error = e.SocketErrorCode;
                                    }

                                    if (error == SocketError.Success)
                                        Task.Delay(PING_TIMEOUT_MS).ContinueWith(ping);
                                    else
                                        OnDisconnected(new DisconnectEventArgs() { Error = error });
                                };

                            Task.Delay(PING_TIMEOUT_MS).ContinueWith(ping);
                        }

                        return task.Result;
                    });
        }

        private void ConnectCompleted(Object sender, SocketAsyncEventArgs e)
        {
            if (e.SocketError != SocketError.Success)
                LastError = e.SocketError;

            connectsignal.Set();

            if (!socket.Connected)
                OnDisconnected(new DisconnectEventArgs() { Error = e.SocketError });
        }

        public Task<bool> Authenticate(UInt64 UserId, UInt64 DeviceId, UInt64 Secret)
        {
            var msg = new AuthenticateMessage()
            {
                user_id = UserId,
                device_id = DeviceId,
                secret = Secret,
            };

            var signal = new AutoResetEvent(false);
            Action<Dictionary<String, Object>> handler = dictionary =>
            {
                token = Convert.ToUInt64(dictionary["token"]);
                signal.Set();
            };

            lock (rpchandler) rpchandler[(UInt16)MessageType.AUTHENTICATE_RESPONSE].Enqueue(handler);

            return Send(serializer.Serialize(msg))
                .ContinueWith(task =>
                {
                    bool proceed = task.Result;
                    if (proceed) signal.WaitOne(); // block until the next response
                    return proceed;
                });
        }

        public class NotifyEventArgs : EventArgs
        {
            public UInt64 ContentId;
            public List<String> ContentType;
            public String Description;
        }

        public event EventHandler<NotifyEventArgs> ContentAvailable;

        protected void OnContentAvailable(NotifyEventArgs e)
        {
            if (ContentAvailable != null)
                ContentAvailable(this, e);
        }

        public Task<bool> Notify(UInt64 ContentId, List<String> ContentType, String Description)
        {
            lock (contentCache)
            {
                if (contentCache.ContainsKey(ContentId) == false)
                    contentCache.Add(ContentId, Description);
            }

            var msg = new NotifyMessage()
            {
                token = token,
                dataset_id = (UInt16)DatasetType.CLIPBOARD,
                content_id = ContentId,
                content_type = ContentType,
                description = Description,
            };

            return Send(serializer.Serialize(msg));
        }

        public Task<byte[]> Sync(UInt64 ContentId)
        {
            Console.WriteLine("attempting to sync " + ContentId);

            var msg = new SyncMessage()
            {
                token = token,
                content_id = ContentId,
                dataset_id = (UInt16)DatasetType.CLIPBOARD
            };

            byte[] content = null;
            var signal = new AutoResetEvent(false);
            Action<Dictionary<String, Object>> handler = dictionary =>
            {
                var content_size = Convert.ToUInt32(dictionary["content_size"]);
                var buffer = new byte[content_size];
                if (Recv(buffer, buffer.Length))
                    content = buffer;

                signal.Set();
            };

            lock (rpchandler) rpchandler[(UInt16)MessageType.SYNC_RESPONSE].Enqueue(handler);

            return Send(serializer.Serialize(msg))
                .ContinueWith(task =>
                    {
                        if (task.Result) signal.WaitOne(); // block until the next response
                        return content;
                    });
        }

        private Task<bool> Send(String message)
        {
            return Task.Factory.StartNew(() =>
            {
                Console.WriteLine("--> " + message);

                bool proceed = sendbuf == null && sendbytes == 0; // TODO: queue for concurrent sends

                if (proceed)
                {
                    sendev.SetBuffer(buffer, 0, SEND_BUFFER_SIZE);

                    sendbytes = 0;
                    sendbuf = Encoding.UTF8.GetBytes(message);

                    proceed = proceed &&
                        sendbuf.Length <= UInt16.MaxValue &&
                        SendHeader(new MessageHeader() { payload_size = (Int16)sendbuf.Length });

                    if (proceed)
                    {
                        if (!SendContinue())
                            SendCompleted(this, sendev);

                        sendsignal.WaitOne();

                        proceed = sendbytes == sendbuf.Length;
                        sendbuf = null;
                        sendbytes = 0;
                    }
                }

                return proceed;
            });
        }

        private Task<bool> SendContent(String content)
        {
            return Task.Factory.StartNew(() =>
            {
                Console.WriteLine("--> " + content);

                bool proceed = sendbuf == null && sendbytes == 0; // TODO: queue for concurrent sends

                if (proceed)
                {
                    sendev.SetBuffer(buffer, 0, SEND_BUFFER_SIZE);

                    sendbytes = 0;
                    sendbuf = Encoding.UTF8.GetBytes(content);

                    if (!SendContinue())
                        SendCompleted(this, sendev);

                    sendsignal.WaitOne();

                    proceed = sendbytes == sendbuf.Length;
                    sendbuf = null;
                    sendbytes = 0;
                }

                return proceed;
            });
        }

        private bool SendHeader(MessageHeader header)
        {
            bool success = false;
            
            try
            {
                var payload_size = BitConverter.GetBytes(IPAddress.HostToNetworkOrder(header.payload_size));
                var sent = socket.Send(payload_size);
                success = header.payload_size > 0 && sent == sizeof(Int16);
            }
            catch (SocketException e)
            {
                OnDisconnected(new DisconnectEventArgs() { Error = e.SocketErrorCode });
            }

            return success;
        }

        private bool SendContinue()
        {
            var size = Math.Min(sendbuf.Length - sendbytes, SEND_BUFFER_SIZE);
            Buffer.BlockCopy(sendbuf, sendbytes, sendev.Buffer, sendev.Offset, size);
            sendev.SetBuffer(sendev.Offset, size);

            return socket.SendAsync(sendev);
        }

        private void SendCompleted(Object sender, SocketAsyncEventArgs e)
        {
            bool success = e.SocketError == SocketError.Success;
            sendbytes += success ? e.BytesTransferred : 0;

            bool proceed = success && sendbytes < sendbuf.Length;
            if (proceed && !SendContinue())
                SendCompleted(this, sendev);

            if (!success) LastError = e.SocketError;
            if (!proceed) sendsignal.Set();

            if (!socket.Connected)
                OnDisconnected(new DisconnectEventArgs() { Error = e.SocketError });
        }

        private bool Recv(byte[] outbuffer, int size)
        {
            Console.WriteLine("trying to read " + size);
            bool proceed = recvbuf == null && recvbytes == 0 && recvtotal == 0 && // TODO: queue for concurrent recvs
                outbuffer.Length >= size;

            if (proceed)
            {
                recvbytes = 0;
                recvtotal = size;
                recvbuf = outbuffer;

                if (!RecvContinue())
                    RecvCompleted(this, recvev);

                recvsignal.WaitOne();

                proceed = recvbytes == recvtotal;

                recvbytes = 0;
                recvtotal = 0;
                recvbuf = null;
            }

            return proceed;
        }

        private bool RecvHeader(byte[] buffer, MessageHeader header)
        {
            header.payload_size = Recv(buffer, sizeof(Int16)) ?
                IPAddress.NetworkToHostOrder(BitConverter.ToInt16(buffer, 0)) : (Int16)0;

            return header.payload_size > 0;
        }

        private bool RecvContinue()
        {
            if (recvremaining > 0)
                return false; // still have bytes left in socket buffer
            else
            {
                recvcopied = 0;
                recvremaining = 0;
                recvev.SetBuffer(buffer, SEND_BUFFER_SIZE, RECV_BUFFER_SIZE);
                return socket.ReceiveAsync(recvev);
            }
        }

        private void RecvCompleted(Object sender, SocketAsyncEventArgs e)
        {
            bool success = e.SocketError == SocketError.Success;
            bool proceed = success && e.BytesTransferred > 0;
            Console.WriteLine("<<<<< error: " + e.SocketError + " transferred: " + e.BytesTransferred);

            if (proceed)
            {
                if (recvremaining == 0)
                    recvremaining = e.BytesTransferred;

                var size = Math.Min(recvtotal - recvbytes, recvremaining);
                Buffer.BlockCopy(recvev.Buffer, recvev.Offset + recvcopied, recvbuf, recvbytes, size);

                recvremaining -= size;
                recvcopied += size;
                recvbytes += size;
                proceed = recvbytes < recvtotal;

                if (proceed && !RecvContinue())
                    RecvCompleted(this, sendev);
            }

            if (!success) LastError = e.SocketError;
            if (!proceed) recvsignal.Set();

            if (!socket.Connected)
                OnDisconnected(new DisconnectEventArgs() { Error = e.SocketError });
        }
    }
}
