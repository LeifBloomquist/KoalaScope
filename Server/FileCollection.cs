using System.Xml.Serialization;

namespace KoalaScopeServer
{
    public class FileCollection
    {
        private FileSystemWatcher watcher = new();
        private List<string> FileNames = [];
        private readonly Object LockObj = new();

        public FileCollection(string path, string filter)
        {
            watcher.Path = path;

            watcher.NotifyFilter = NotifyFilters.Attributes
                                 | NotifyFilters.CreationTime
                                 | NotifyFilters.DirectoryName
                                 | NotifyFilters.FileName
                                 | NotifyFilters.LastWrite
                                 | NotifyFilters.Security
                                 | NotifyFilters.Size;

            watcher.Filter = filter;
            watcher.Changed += OnChanged;
            watcher.Created += OnChanged;
            watcher.Deleted += OnChanged;
            watcher.Renamed += OnChanged;
            watcher.EnableRaisingEvents = true;

            ScanDirectory();
        }

        private void OnChanged(object source, FileSystemEventArgs e)
        {
            // Something changed - Scan the directory again
            ScanDirectory();
        }

        public void Dispose()
        {
            // avoiding resource leak
            watcher.Changed -= OnChanged;
            this.watcher.Dispose();
        }

        private void ScanDirectory()
        {
            lock (LockObj)
            {
                FileNames.Clear();
                FileNames.AddRange([.. Directory.GetFiles(watcher.Path, watcher.Filter)]);
                Console.WriteLine("Directory [" + watcher.Path + "] scanned.  Number of entries matching filter [" + watcher.Filter + "]: " + FileNames.Count);
            }
        }

        public List<String> GetFileNames()
        {
            lock (LockObj)
            {
                return [.. FileNames];   // Copy
            }
        }

        public UInt32 GetFileCount()
        {
            lock (LockObj)
            {
                return (UInt32)FileNames.Count;
            }
        }
        public byte[] GetFileContents(int index)
        {
            lock (LockObj)
            {
                return File.ReadAllBytes(FileNames[index]);
            }
        }
    }
}
