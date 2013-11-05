try {
	module.exports = require('./build/Release/quvi')
} catch (e) {
	try {
		module.exports = require('./build/Debug/quvi')
	} catch (e) {
		throw new Error('Native binary not found')
	}
}
