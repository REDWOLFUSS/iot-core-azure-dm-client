﻿using Microsoft.Azure.Devices.Client;
using Microsoft.Azure.Devices.Shared;
using Microsoft.Devices.Management;
using System;
using System.Diagnostics;
using System.Threading.Tasks;
using Windows.UI.Core;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

namespace Toaster
{
    public sealed partial class MainPage : Page
    {
        DeviceManagementClient deviceManagementClient;

        private const string DeviceConnectionString = "HostName=iot-open-house-demo.azure-devices.net;DeviceId=dog-feeder;SharedAccessKey=Rt3JTLyI3Rj4ZCNm6pPkT8TKHTw1TMLMNnXd8u/pp9g=";

        public MainPage()
        {
            this.InitializeComponent();
            this.buttonStart.IsEnabled = true;
            this.buttonStop.IsEnabled = false;
            this.imageHot.Visibility = Visibility.Collapsed;
            DeviceClient deviceClient = DeviceClient.CreateFromConnectionString(DeviceConnectionString, TransportType.Mqtt);

            this.deviceManagementClient = DeviceManagementClient.Create(
                new AzureIoTHubDeviceTwinProxy(deviceClient), 
                new ToasterDeviceManagementRequestHandler(this));

            deviceClient.SetDesiredPropertyUpdateCallback(OnDesiredPropertyUpdate, null);
        }

        public Task OnDesiredPropertyUpdate(TwinCollection desiredProperties, object userContext)
        {
            this.deviceManagementClient.ProcessDeviceManagementProperties(desiredProperties);

            // Application developer can process all the top-level nodes here

            return Task.CompletedTask;
        }

        // This method may get called on the DM callback thread - not on the UI thread.
        public async Task<bool> YesNo(string question)
        {
            var tcs = new TaskCompletionSource<bool>();

            await Dispatcher.RunAsync(CoreDispatcherPriority.Normal, async () =>
            {
                UserDialog dlg = new UserDialog(question);
                ContentDialogResult dialogResult = await dlg.ShowAsync();
                tcs.SetResult(dlg.Result);
            });

            bool yesNo = await tcs.Task;
            return yesNo;
        }

        private void OnStartToasting(object sender, RoutedEventArgs e)
        {
            this.buttonStart.IsEnabled = false;
            this.buttonStop.IsEnabled = true;
            this.slider.IsEnabled = false;
            this.textBlock.Text = string.Format("Toasting at {0}%", this.slider.Value);
            this.imageHot.Visibility = Visibility.Visible;
        }

        private void OnStopToasting(object sender, RoutedEventArgs e)
        {
            this.buttonStart.IsEnabled = true;
            this.buttonStop.IsEnabled = false;
            this.slider.IsEnabled = true;
            this.textBlock.Text = "Ready";
            this.imageHot.Visibility = Visibility.Collapsed;
        }

        private async void OnCheckForUpdates(object sender, RoutedEventArgs e)
        {
            bool updatesAvailable = await deviceManagementClient.CheckForUpdatesAsync();
            if (updatesAvailable)
            {
                System.Diagnostics.Debug.WriteLine("updates available");
                var dlg = new UserDialog("Updates available. Install?");
                await dlg.ShowAsync();
                // Don't do anything yet
            }
        }

        private async void RestartSystem()
        {
            bool success = true;
            try
            {
                await deviceManagementClient.ImmediateRebootAsync();
            }
            catch(Exception)
            {
                success = false;
            }

            StatusText.Text = success?  "Operation completed" : "Operation  failed";
        }

        private void OnSystemRestart(object sender, RoutedEventArgs e)
        {
            RestartSystem();
        }

        private async void FactoryReset()
        {
            bool success = true;
            try
            {
                await deviceManagementClient.DoFactoryResetAsync();
            }
            catch (Exception)
            {
                success = false;
            }

            StatusText.Text = success? "Succeeded!" : "Failed!";
         }

        private void OnFactoryReset(object sender, RoutedEventArgs e)
        {
            FactoryReset();
        }
    }
}