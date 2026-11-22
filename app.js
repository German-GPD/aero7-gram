(() => {
  "use strict";

  const year = document.getElementById("year");
  if (year) year.textContent = String(new Date().getFullYear());

  const status = document.getElementById("release-status");
  const meta = document.getElementById("release-meta");
  const checksums = document.getElementById("release-checksums");
  if (!status || !meta) return;

  const repo = "German-GPD/aero7-gram";
  const releasePage = `https://github.com/${repo}/releases/tag/test-latest`;
  const assetNames = ["AERO7-GRAM-x64-Setup.exe", "AERO7-GRAM-x86-Setup.exe"];

  const setStatus = (message, state) => {
    status.dataset.state = state;
    status.querySelector("span:last-child").textContent = message;
  };

  const humanSize = bytes => {
    if (!Number.isFinite(bytes) || bytes <= 0) return "Размер не указан";
    return bytes >= 1024 * 1024
      ? `${(bytes / 1024 / 1024).toFixed(1)} MB`
      : `${Math.round(bytes / 1024)} KB`;
  };

  fetch(`https://api.github.com/repos/${repo}/releases/tags/test-latest`, {
    headers: { Accept: "application/vnd.github+json" }
  })
    .then(response => {
      if (!response.ok) throw new Error(`Release API: ${response.status}`);
      return response.json();
    })
    .then(release => {
      const assets = Array.isArray(release.assets) ? release.assets : [];
      const byName = new Map(assets.map(asset => [asset.name, asset]));
      let available = 0;

      assetNames.forEach(name => {
        const link = document.querySelector(`[data-release-asset="${name}"]`);
        if (!link) return;
        const asset = byName.get(name);
        const small = link.querySelector("small");
        if (asset && asset.browser_download_url) {
          link.href = asset.browser_download_url;
          link.setAttribute("download", "");
          link.setAttribute("aria-label", `Download ${name}`);
          link.classList.remove("is-unavailable");
          if (small) small.textContent = `${humanSize(asset.size)} · Installer · SHA-256 below`;
          available += 1;
        } else {
          link.href = releasePage;
          link.classList.add("is-unavailable");
          link.setAttribute("aria-label", `${name} is not published yet; open release page`);
          if (small) small.textContent = "Сборка пока не опубликована";
        }
      });

      const date = release.published_at
        ? new Date(release.published_at).toLocaleDateString("ru-RU", { year: "numeric", month: "long", day: "numeric" })
        : "дата не указана";
      meta.textContent = `VERSION ${release.tag_name || "TEST"} · ${date.toUpperCase()} · ${available}/2 INSTALLERS AVAILABLE`;
      setStatus(
        available === 2 ? "Обе сборки готовы к загрузке" :
        available === 1 ? "Опубликована одна архитектура; вторая ещё собирается" :
        "Установщики пока не опубликованы — открой страницу релиза",
        available === 2 ? "ready" : "pending"
      );

      if (checksums) {
        const checksumNames = assetNames.map(name => `${name}.sha256`);
        const checksumAssets = checksumNames.map(name => byName.get(name)).filter(Boolean);
        checksums.replaceChildren();
        if (checksumAssets.length) {
          const label = document.createElement("span");
          label.textContent = "SHA-256: ";
          checksums.append(label);
          checksumAssets.forEach(asset => {
            const a = document.createElement("a");
            a.href = asset.browser_download_url;
            a.textContent = asset.name;
            a.target = "_blank";
            a.rel = "noopener";
            checksums.append(a);
          });
        } else {
          checksums.textContent = "SHA-256 контрольные суммы будут доступны вместе с установщиками.";
        }
      }
    })
    .catch(() => {
      setStatus("Не удалось проверить релиз. Открой страницу GitHub Releases.", "pending");
      meta.textContent = "GITHUB RELEASES · TEST CHANNEL";
      document.querySelectorAll("[data-release-asset]").forEach(link => {
        link.href = releasePage;
        link.classList.add("is-unavailable");
      });
    });
})();