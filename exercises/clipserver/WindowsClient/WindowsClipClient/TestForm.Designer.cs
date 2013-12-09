namespace ClipClient
{
    partial class TestForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.toWirelessButton = new System.Windows.Forms.Button();
            this.toSystemButton = new System.Windows.Forms.Button();
            this.deleteButton = new System.Windows.Forms.Button();
            this.listView1 = new System.Windows.Forms.ListView();
            this.SuspendLayout();
            // 
            // toWirelessButton
            // 
            this.toWirelessButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.toWirelessButton.AutoSize = true;
            this.toWirelessButton.FlatAppearance.BorderColor = System.Drawing.Color.FromArgb(((int)(((byte)(192)))), ((int)(((byte)(0)))), ((int)(((byte)(0)))));
            this.toWirelessButton.FlatStyle = System.Windows.Forms.FlatStyle.Popup;
            this.toWirelessButton.Location = new System.Drawing.Point(12, 227);
            this.toWirelessButton.Name = "toWirelessButton";
            this.toWirelessButton.Size = new System.Drawing.Size(77, 23);
            this.toWirelessButton.TabIndex = 0;
            this.toWirelessButton.Text = "Wireless Clip";
            this.toWirelessButton.UseVisualStyleBackColor = true;
            this.toWirelessButton.Click += new System.EventHandler(this.toWirelessButton_Click);
            // 
            // toSystemButton
            // 
            this.toSystemButton.Anchor = System.Windows.Forms.AnchorStyles.Bottom;
            this.toSystemButton.FlatAppearance.BorderColor = System.Drawing.Color.FromArgb(((int)(((byte)(192)))), ((int)(((byte)(0)))), ((int)(((byte)(0)))));
            this.toSystemButton.FlatStyle = System.Windows.Forms.FlatStyle.Popup;
            this.toSystemButton.Location = new System.Drawing.Point(95, 227);
            this.toSystemButton.Name = "toSystemButton";
            this.toSystemButton.Size = new System.Drawing.Size(118, 23);
            this.toSystemButton.TabIndex = 1;
            this.toSystemButton.Text = "System Clipboard";
            this.toSystemButton.UseVisualStyleBackColor = true;
            this.toSystemButton.Click += new System.EventHandler(this.toSystemButton_Click);
            // 
            // deleteButton
            // 
            this.deleteButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.deleteButton.FlatAppearance.BorderColor = System.Drawing.Color.FromArgb(((int)(((byte)(192)))), ((int)(((byte)(0)))), ((int)(((byte)(0)))));
            this.deleteButton.FlatStyle = System.Windows.Forms.FlatStyle.Popup;
            this.deleteButton.Location = new System.Drawing.Point(219, 227);
            this.deleteButton.Name = "deleteButton";
            this.deleteButton.Size = new System.Drawing.Size(75, 23);
            this.deleteButton.TabIndex = 2;
            this.deleteButton.Text = "Delete";
            this.deleteButton.UseVisualStyleBackColor = true;
            this.deleteButton.Click += new System.EventHandler(this.deleteButton_Click);
            // 
            // listView1
            // 
            this.listView1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.listView1.BackColor = System.Drawing.Color.Snow;
            this.listView1.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.listView1.Location = new System.Drawing.Point(12, 12);
            this.listView1.MultiSelect = false;
            this.listView1.Name = "listView1";
            this.listView1.Size = new System.Drawing.Size(282, 209);
            this.listView1.TabIndex = 4;
            this.listView1.UseCompatibleStateImageBehavior = false;
            this.listView1.View = System.Windows.Forms.View.List;
            this.listView1.SelectedIndexChanged += new System.EventHandler(this.listView1_SelectedIndexChanged);
            // 
            // TestForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.AutoScroll = true;
            this.BackColor = System.Drawing.SystemColors.Control;
            this.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Center;
            this.ClientSize = new System.Drawing.Size(306, 262);
            this.Controls.Add(this.listView1);
            this.Controls.Add(this.deleteButton);
            this.Controls.Add(this.toSystemButton);
            this.Controls.Add(this.toWirelessButton);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.SizableToolWindow;
            this.MinimumSize = new System.Drawing.Size(322, 296);
            this.Name = "TestForm";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.TopMost = true;
            this.Load += new System.EventHandler(this.TestForm_Load);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        private void TestForm_Load(object sender, System.EventArgs e)
        {
        }

        private void listView1_SelectedIndexChanged(object sender, System.EventArgs e)
        {
        }

        #endregion

        private System.Windows.Forms.Button toWirelessButton;
        private System.Windows.Forms.Button toSystemButton;
        private System.Windows.Forms.Button deleteButton;
        private System.Windows.Forms.ListView listView1;
    }
}
