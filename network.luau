local network = {}
      network.LOAD_SO = true
      network.SO_PREFIX = "vlm_"

function network.RequestGet(url: string, headers: {string}) end
function network.RequestPost(url: string, body: string, headers: {string}) end
function network.RequestPostForm(url: string, form: {}) end

return network
