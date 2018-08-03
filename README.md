## zstream-splitter

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**CAUTION: This might eat your data/backups. User beware.**

---

A program to take a zfs stream and split it into chunks. Modified from zstreamdump in the [zfsonlinux](https://github.com/zfsonlinux/zfs) archive.

General intent was to make it easier to upload a zfs send backup to Google Drive. See zstream-split-gdrive for an example of uploading via `rclone`.

```
usage: zstream-splitter [-h] [-b size] [-s size]
	-h        -- show this help
	-b <size> -- specify a breakpoint in bytes, where <size> can be given in b,k,M,G
	-s <size> -- specify amount already transferred in bytes, where <size> can be given in b,k,M,G
	-V        -- display version

  Splits a stream from 'zfs send' and outputs the appropriate
  resume token. STDOUT and STDIN should not be terminals.
```
Outputs `receive_resume_token:` and `bytes:` to STDERR and exits with error code 10 if stream is not completed yet.
### Usage example
```bash
##### send to gzip files
$ sudo zfs send -pLDce rpool/var/log@test-send | \
	zstream-splitter -b30M |gzip \
	> rpool_var_log@test-send.01.gz
	
Splitting approximately every 31457280 bytes
receive_resume_token:	1-11f11177d9-118-789c636064000310a500c4ec50360710e72765a526973030c8409460caa7a515a796806402e0f26c48f2499525a9c5407ac18a3b8cd8f497e4a79766a6303084ffebd25973fd8c8301923c27583e2f31379581a1a8203f3f47bf2cb1483f273fdd016866896e716a5e0ad83c5e0684fb73128bd2539372f293b3f3b3416212507b60f2a9b949a9294029903e6e24f1e4fcdc82a2d4e262882e080000ee0c29d7
bytes:	31238304

$ sudo zfs send -t 1-11f11177d9-118-789c636064000310a500c4ec50360710e72765a526973030c8409460caa7a515a796806402e0f26c48f2499525a9c5407ac18a3b8cd8f497e4a79766a6303084ffebd25973fd8c8301923c27583e2f31379581a1a8203f3f47bf2cb1483f273fdd016866896e716a5e0ad83c5e0684fb73128bd2539372f293b3f3b3416212507b60f2a9b949a9294029903e6e24f1e4fcdc82a2d4e262882e080000ee0c29d7 | \
	zstream-splitter -b30M -s 31238304 | gzip \
	> rpool_var_log@test-send.02.gz
	
Splitting approximately every 31457280 bytes
Adding 31238400 to bytes transferred
zfs send completed

##########################
# Now, receive
$ zcat rpool_var_log@test-send.01.gz | sudo zfs receive -Fsuv rpool/test-receive

receiving full stream of rpool/var/log@test-send into rpool/test-receive@test-send
cannot receive new filesystem stream: checksum mismatch or incomplete stream.
Partially received snapshot is saved.
A resuming stream can be generated on the sending system by running:
    zfs send -t 1-11f11177d9-118-789c636064000310a500c4ec50360710e72765a526973030c8409460caa7a515a796806402e0f26c48f2499525a9c5407ac18a3b8cd8f497e4a79766a6303084ffebd25973fd8c8301923c27583e2f31379581a1a8203f3f47bf2cb1483f273fdd016866896e716a5e0ad83c5e0684fb73128bd2539372f293b3f3b3416212507b60f2a9b949a9294029903e6e24f1e4fcdc82a2d4e262882e080000ee0c29d7

$ zcat rpool_var_log@test-send.02.gz | sudo zfs receive -Fsuv rpool/test-receive

receiving full stream of rpool/var/log@test-send into rpool/test-receive@test-send
received 26.2M stream in 1 seconds (26.2M/sec)

```