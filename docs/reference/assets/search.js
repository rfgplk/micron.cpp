(() => {
  const form = document.getElementById('search-form');
  const input = document.getElementById('query');
  const status = document.getElementById('search-status');
  const results = document.getElementById('search-results');
  const params = new URLSearchParams(window.location.search);
  input.value = params.get('q') || '';

  function search() {
    const words = input.value.trim().toLowerCase().split(/\s+/).filter(Boolean);
    results.replaceChildren();
    if (!words.length) {
      status.textContent = 'Enter a name or header.';
      return;
    }
    const matches = window.MICRON_SEARCH_INDEX.filter(([name, , path]) => {
      const text = (name + ' ' + path).toLowerCase();
      return words.every(word => text.includes(word));
    });
    status.textContent = matches.length + ' result' + (matches.length === 1 ? '' : 's') +
      (matches.length > 100 ? ' (showing the first 100)' : '');
    for (const [name, target, path] of matches.slice(0, 100)) {
      const item = document.createElement('li');
      const link = document.createElement('a');
      link.href = target;
      link.textContent = name;
      const header = document.createElement('code');
      header.textContent = ' <micron/' + path + '>';
      item.append(link, header);
      results.append(item);
    }
  }
  form.addEventListener('submit', event => {
    event.preventDefault();
    const url = new URL(window.location.href);
    url.searchParams.set('q', input.value.trim());
    history.replaceState(null, '', url);
    search();
  });
  if (input.value) search();
})();
