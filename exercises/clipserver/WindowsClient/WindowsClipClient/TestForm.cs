using ClipClient.Properties;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace ClipClient
{
    public partial class TestForm : Form
    {        
        private static int NUM_CLIPS = 5;
        private NotifyIcon tray_icon = new NotifyIcon();
        private ContextMenu tray_menu = new ContextMenu();
        private ClipManager clip_manager = new ClipManager();
        private List<int> local_clips_hash = new List<int>(NUM_CLIPS);

        public TestForm()
        {
            InitializeComponent();

            tray_menu.MenuItems.Add("Exit", OnExit);
            tray_menu.MenuItems.Add("Show Clipboard", ShowClipboard);
            tray_menu.MenuItems.Add("Hide Clipboard", HideClipboard);

            tray_icon.Text = "Wireless Clipboard";
            tray_icon.Icon = Properties.Resources.cloud_clip_light_icon_32;
            tray_icon.ContextMenu = tray_menu;
            tray_icon.Visible = true;
            tray_icon.BalloonTipClicked += new EventHandler(tray_icon_BalloonTipClicked);

            this.FormClosing += HideForm_OnFormClosing;

            clip_manager.ClipboardChanged += (sender, e) => 
                {
                    var content = CoerceContent(e);


                    // Hash here to check for duplicate

                    Console.WriteLine("++++++++++++++++");

                    Console.WriteLine("O String: {0}", content.Description);
                    Console.WriteLine("O Hash:   {0}", content.Description.GetHashCode());

                    String urlA = "http://" + content.Description;

                    Console.WriteLine("A String: {0}", urlA);
                    Console.WriteLine("A Hash:   {0}", urlA.GetHashCode());

                    String urlB = "http://" + content.Description + "/";

                    Console.WriteLine("B String: {0}", urlB);
                    Console.WriteLine("B Hash:   {0}", urlB.GetHashCode());

                    Console.WriteLine("++++++++++++++++");

                    if (!local_clips_hash.Contains(content.Description.GetHashCode()))
                    {
                        clip_manager.Notify(content.Description, content.ContentType);
                        AddContentToForm(content);
                        DisplayBubbleNotification("Clipboard changed", content.Description);
                    }
                };

            clip_manager.ContentSynced += (sender, e) =>
                {
                    var content = CoerceContent(e);
                    AddContentToForm(content);
                    DisplayBubbleNotification("Synced content", content.Description);
                };
        }

        private Content CoerceContent(ClipEventArgs args)
        {
            var content_type = new List<String>();
            var formats = args.Clip.GetFormats();

            foreach (var format in formats)
            {
                switch (format)
                {
                    case "PNG":
                        content_type.Add("image/png");
                        break;  

                    case "DeviceIndependentBitmap":
                        content_type.Add("image/bmp");
                        break;

                    case "FileName":
                    case "FileDrop":
                        content_type.Add("text/path");
                        break;

                    case "HTML Format":
                    case "text/html":
                        content_type.Add("text/html");
                        var html = args.Clip.GetData("HTML Format", true);
                        break;

                    case "Text":
                    case "UnicodeText":
                    case "System.String":
                    case "text/plain":
                        content_type.Add("text/plain");
                        break;
                }
            }

            content_type = content_type.Distinct().ToList(); // eliminate duplicates
            var description = "Unable to find text description.";

            if (content_type.Contains("text/path"))
            {
                var builder = new StringBuilder();
                var files = args.Clip.GetData("FileName", true) as String[];
                foreach (var file in files)
                {
                    builder.Append(file);
                    builder.Append(';');
                }
                description = builder.ToString();
            }
            else if (content_type.Contains("text/plain"))
            {
                description = args.Clip.GetData("System.String", true) as String;
            }
            else if (content_type.Contains("text/html"))
            {
                description = args.Clip.GetData("HTML Format", true) as String;
            }

            return new Content () { Clip = args.Clip, ContentType = content_type, Description = description };
        }

        private Content CoerceContent(SyncEventArgs args)
        {
            return new Content() { Clip = null, ContentType = args.ContentType, Description = args.Content };
        }

        private void AddContentToForm(Content content)
        {
            if (listView1.InvokeRequired)
                Invoke(new Action(() => AddContentToForm(content)));
            else
            {
                lock (listView1.Items)
                {
                    listView1.Items.Add(new ListViewItem(content.Description) { Tag = content });
                    local_clips_hash.Add(content.Description.GetHashCode());
                    if (listView1.Items.Count > NUM_CLIPS)
                        listView1.Items.RemoveAt(0);
                }
            }            
        }

        private void DisplayBubbleNotification(String title, String text)
        {
            tray_icon.BalloonTipIcon = ToolTipIcon.Info;
            tray_icon.BalloonTipTitle = title;
            tray_icon.BalloonTipText = text;
            tray_icon.ShowBalloonTip(1000);
        }

        private void tray_icon_BalloonTipClicked(object sender, EventArgs e)
        {
            this.Show();
        }

        private void OnExit(Object sender, EventArgs e)
        {
            // TODO: Save local clips
            Application.Exit();
        }

        private void ShowClipboard(Object sender, EventArgs e)
        {
            this.Show();
            this.BringToFront();
        }

        private void HideClipboard(Object sender, EventArgs e)
        {
            this.Hide();
        }

        protected override void OnLoad(EventArgs e)
        {
            Visible = false;
            ShowInTaskbar = false;
            base.OnLoad(e);
        }

        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                if (components != null)
                {
                    components.Dispose();
                }
                tray_icon.Dispose();
            }
            base.Dispose(disposing);
        }

        // Overrides FormClose. This keep the clipboard around for when we want to get is back via the notification bar
        private void HideForm_OnFormClosing(object sender, FormClosingEventArgs e)
        {
            if (e.CloseReason == CloseReason.ApplicationExitCall)
            {
                e.Cancel = false;
                this.Close();
            }
            else
            {
                e.Cancel = true;
                this.Hide();
            }
        }

        private void toWirelessButton_Click(object sender, EventArgs e)
        {
            if (listView1.SelectedItems.Count > 0)
            {
                DisplayBubbleNotification("Wireless Clipboard", "Sent to wireless clipboard");
                var content = (Content) listView1.SelectedItems[0].Tag;
                if (content.Clip != null)
                    clip_manager.Notify(content.Description, content.ContentType);
            }
        }

        private void toSystemButton_Click(object sender, EventArgs e)
        {
            if (listView1.SelectedItems.Count > 0)
            {
                DisplayBubbleNotification("Wireless Clipboard", "Sent to system clipboard");
                var text = listView1.SelectedItems[0].Text;
                Clipboard.SetText(text);
            }
        }

        private void deleteButton_Click(object sender, EventArgs e)
        {
            if (listView1.SelectedItems.Count > 0)
            {
                DisplayBubbleNotification("Wireless Clipboard", "Clip deleted");
                lock (listView1.Items)
                {
                    Console.WriteLine("D String: {0}", listView1.SelectedItems[0].Text);
                    Console.WriteLine("D Hash:   {0}", listView1.SelectedItems[0].Text.GetHashCode());

                    local_clips_hash.Remove(listView1.SelectedItems[0].Text.GetHashCode());
                    listView1.SelectedItems[0].Remove();
                }
            }
        }
    }

    public class Content
    {
        public IDataObject Clip = null;
        public String Description = null;
        public List<String> ContentType = null;
    }

}
