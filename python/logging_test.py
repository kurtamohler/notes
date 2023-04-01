import logging

log = logging.getLogger(__name__)
log.addHandler(logging.StreamHandler())

log.setLevel(logging.INFO)

log.warning('this is a warning')
log.info('this is an info')


