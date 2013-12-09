using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Net;
using System.Threading;

namespace ClipClient
{
    public class ClipEventArgs : System.EventArgs
    {
        public IDataObject Clip;
    }
    
    public class SyncEventArgs : System.EventArgs
    {
        public String Content;
        public List<String> ContentType;
    }

    public class ClipManager
    {
        public event EventHandler<ClipEventArgs> ClipboardChanged;
        public event EventHandler<SyncEventArgs> ContentSynced;

        protected void OnClipboardChanged(ClipEventArgs e)
        {
            if (ClipboardChanged != null)
                ClipboardChanged(this, e);
        }

        protected void OnContentSynced(SyncEventArgs e)
        {
            if (ContentSynced != null)
                ContentSynced(this, e);
        }

        private MessageManager messagemgr = new MessageManager();
        private volatile bool reconnecting = false;
        private const int RECONNECT_TIMEOUT_MS = 3000;

        public ClipManager()
        {
            IPAddress address = IPAddress.Parse(Properties.Resources.server_address);
            int port = Convert.ToInt16(Properties.Resources.server_port);
            ulong user_id = Util.GenerateHash63(Properties.Resources.account_name);
            ulong device_id = Util.GenerateHash63(Properties.Resources.device_name);
            ulong secret = Util.GenerateHash63(Properties.Resources.account_secret);

            if (Properties.Settings.Default.account_name.Length > 0)
            {
                Console.WriteLine("have setting account name: " + Properties.Settings.Default.account_name);
                user_id = Util.GenerateHash63(Properties.Settings.Default.account_name);
            }

            if (Properties.Settings.Default.device_name.Length > 0)
            {
                Console.WriteLine("have setting device name: " + Properties.Settings.Default.device_name);
                device_id = Util.GenerateHash63(Properties.Settings.Default.device_name);
            }

            ClipNotification.ClipboardChanged += (sender, e) =>
            {
                OnClipboardChanged(new ClipEventArgs() { Clip = e.Clip }); // relay clipboard changes out
            };

            messagemgr.ContentAvailable += (sender, e) =>
                {
                    Console.WriteLine("notified of clip with description: " + e.Description);

                    Task.Factory.StartNew(() =>
                        {
                            var task = messagemgr.Sync(e.ContentId);
                            if (task.Result != null)
                            {
                                String clipText =  Encoding.UTF8.GetString(task.Result);
                                OnContentSynced(new SyncEventArgs() { Content = clipText, ContentType = e.ContentType });
                                Console.WriteLine("content: " + clipText);
                            }                                
                        });
                };

            messagemgr.Disconnected += (sender, e) =>
                {
                        if (!reconnecting && !messagemgr.IsConnected)
                        {
                            reconnecting = true;
                            Action<Task> reconnect = null;
                            reconnect = t =>
                                {
                                    Console.WriteLine("RECONNECTING...");
                                    bool success =
                                        messagemgr.Connect(address, port).Result &&
                                        messagemgr.Authenticate(user_id, device_id, secret).Result;

                                    reconnecting = false;
                                };

                            Task.Delay(RECONNECT_TIMEOUT_MS).ContinueWith(reconnect);
                        }
                };

            bool result =
                messagemgr.Connect(address, port).Result &&
                messagemgr.Authenticate(user_id, device_id, secret).Result;

            Console.WriteLine("login: " + result);
        }

        public void Notify(String description, List<String> content_type)
        {
            var content_id = Util.GenerateRand63();
            messagemgr.Notify(content_id, content_type, description);
        }
    }
}
