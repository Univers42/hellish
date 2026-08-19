/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   update_install.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/20 00:50:00 by marvin            #+#    #+#             */
/*   Updated: 2026/08/20 00:50:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "update.h"
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

/* Download one URL to `dest`. --proto pins the transfer to http(s) so a
   redirect cannot walk us onto file:// or scp://, and -f makes curl fail on
   an HTTP error instead of writing the error page into the file.

   `min` is the smallest size that makes sense for what is being fetched: a
   release binary under a kilobyte is a truncated download or an error page,
   but a .sha256 file is about eighty bytes, and applying the binary's floor
   to it silently turned "checksum available" into "no checksum published". */
int	update_download(const char *url, const char *dest, long min)
{
	char		out[64];
	char *const	argv[] = {"curl", "-fsSL", "--proto", "=https,http",
		"--max-time", "60", "-o", (char *)dest, (char *)url, NULL};
	struct stat	st;

	if (update_capture(argv, out, sizeof(out)) < 0)
		return (0);
	if (stat(dest, &st) != 0 || st.st_size < min)
		return (0);
	return (1);
}

/* First whitespace-delimited field of a file -- both `sha256sum` output and
   a published .sha256 file are "<hex>  <name>". */
static int	first_field(const char *path, char *out, size_t n)
{
	int		fd;
	ssize_t	r;
	size_t	i;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return (0);
	r = read(fd, out, n - 1);
	close(fd);
	if (r <= 0)
		return (0);
	out[r] = '\0';
	i = 0;
	while (out[i] && out[i] != ' ' && out[i] != '\t' && out[i] != '\n')
		i++;
	out[i] = '\0';
	return (i == 64);
}

/* sha256 of a local file, via sha256sum. 0 when the tool is missing, which
   the caller must treat as "cannot verify", never as "verified". */
static int	sha256_of(const char *path, char *out, size_t n)
{
	char		buf[256];
	char *const	argv[] = {"sha256sum", (char *)path, NULL};
	size_t		i;

	if (update_capture(argv, buf, sizeof(buf)) <= 0)
		return (0);
	i = 0;
	while (buf[i] && buf[i] != ' ' && i + 1 < n)
	{
		out[i] = buf[i];
		i++;
	}
	out[i] = '\0';
	return (i == 64);
}

/* Check `file` against the checksum the release publishes next to the asset.
   Returns 1 verified, 0 REJECTED (published but wrong, or we cannot compute
   one), -1 when the release publishes no checksum at all.  The three are
   kept apart on purpose: -1 is a weaker release, 0 is a corrupt or tampered
   download and must stop the install. */
int	update_verify_sha(const char *tag, const char *asset, const char *file)
{
	char	url[1024];
	char	sumfile[1024];
	char	want[128];
	char	got[128];

	ft_strlcpy(sumfile, file, sizeof(sumfile));
	ft_strlcat(sumfile, ".sha256", sizeof(sumfile));
	ft_strlcpy(url, asset, sizeof(url));
	ft_strlcat(url, ".sha256", sizeof(url));
	if (!update_asset_url(tag, url, url + 512, sizeof(url) - 512))
		return (-1);
	if (!update_download(url + 512, sumfile, 32))
		return (unlink(sumfile), -1);
	if (!first_field(sumfile, want, sizeof(want)))
		return (unlink(sumfile), -1);
	unlink(sumfile);
	if (!sha256_of(file, got, sizeof(got)))
		return (0);
	return (ft_strcmp(want, got) == 0);
}
